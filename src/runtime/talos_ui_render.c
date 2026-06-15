#include "../gui/talos_gui.h"

#include "globals.h"
#include "talos_state.h"
#include <inttypes.h>
#include <signal.h>

void talos_ui_render_dashboard(struct talos_ctx *ctx,
                               talos_state      *state,
                               talos_proc_list  *proc_list,
                               u32               width,
                               u32               height)
{
    if (ctx == nullptr || state == nullptr)
    {
        return;
    }

    static i32  selected_pid    = -1;
    static bool show_kill_popup = false;

    static char selected_proc_name[VX_BUF_SIZE_256] = {0};

    talos_gui_set_next_window_pos(0.0f, 0.0f);
    talos_gui_set_next_window_size((f32) width, (f32) height);

    talos_gui_set_next_window_bg_alpha(0.0f);

    i32 canvas_flags =
        (i32) (TALOS_GUI_WINDOW_NO_DECORATION | TALOS_GUI_WINDOW_NO_MOVE |
               TALOS_GUI_WINDOW_NO_BRING_TO_FRONT_ON_FOCUS | TALOS_GUI_WINDOW_NO_INPUTS);

    if (!talos_gui_begin("TalosMaster", nullptr, canvas_flags))
    {
        talos_gui_end();
        return;
    }

    talos_gui_push_font_large();

    char header_buf[VX_BUF_SIZE_128];
    snprintf(header_buf, sizeof(header_buf), "Talos System Monitor | %s", state->cpu.model);
    talos_gui_text(header_buf);

    talos_gui_pop_font();

    talos_gui_separator();
    talos_gui_spacing();

    f32 left_margin  = 16.0f;
    f32 padding      = 16.0f;
    f32 usable_width = (f32) width - (left_margin * 2.0f);
    f32 card_width   = (usable_width - padding) * 0.5f;

    i32 card_flags = (i32) (TALOS_GUI_WINDOW_NO_TITLE_BAR | TALOS_GUI_WINDOW_NO_RESIZE |
                            TALOS_GUI_WINDOW_NO_MOVE);

    f32 left_column_x = left_margin;
    f32 half_height   = ((f32) height - 80.0f - padding) * 0.5f;

    talos_gui_set_next_window_pos(left_column_x, 60.0f);
    talos_gui_set_next_window_size(card_width, (f32) half_height);

    // TODO: cluster core  usage 0-3 or 0-7 if cores > 32
    // or NUMA nodes | CCX units
    talos_gui_push_font_small();
    if (talos_gui_begin("CPUCardWrapper", nullptr, card_flags))
    {
        talos_gui_text("Processor Telemetry");
        talos_gui_separator();
        talos_gui_spacing();

        f32 aggregate_load = state->cpu.usage[0];

        char agg_text[VX_BUF_SIZE_64];
        snprintf(agg_text, sizeof(agg_text), "Total Workload: %.1f%%", aggregate_load);
        talos_gui_text(agg_text);

        talos_gui_progress_bar(aggregate_load / 100.0f, "");
        talos_gui_spacing();
        talos_gui_separator();
        talos_gui_spacing();

        for (u32 i = 0; i < state->cpu.core_count; ++i)
        {
            f32 core_load = state->cpu.usage[i + 1];

            char core_fmt[32];
            snprintf(core_fmt, sizeof(core_fmt), "Core %-2u", i);
            talos_gui_text(core_fmt);

            talos_gui_same_line();
            talos_gui_progress_bar(core_load / 100.0f, "");
        }
    }
    talos_gui_end();

    // Process card — bottom left

    talos_gui_set_next_window_pos(left_column_x, 60.0f + half_height + padding);
    talos_gui_set_next_window_size(card_width, half_height);

    i32 proc_card_flags =
        card_flags | TALOS_GUI_WINDOW_NO_SCROLLBAR | TALOS_GUI_WINDOW_NO_FOCUS_ON_APPEARING;

    if (talos_gui_begin("ProcessCardWrapper", nullptr, proc_card_flags))
    {
        talos_gui_text("Active Process Table");
        talos_gui_separator();
        talos_gui_spacing();

        f32 available_table_space = half_height - 65.0f;

        if (talos_gui_begin_child("ProcScrollRegion", 0.0f, available_table_space, 0))
        {
            i32 table_flags = TALOS_TABLE_FLAG_BORDERS_INNER_V | TALOS_TABLE_FLAG_ROW_BG |
                              TALOS_TABLE_FLAG_SCROLL_Y | TALOS_TABLE_FLAG_RESIZABLE |
                              TALOS_TABLE_FLAG_SORTABLE;

            if (talos_gui_begin_table("proctable", 5, table_flags, 0.0f))
            {
                talos_gui_table_setup_scroll_freeze(0, 1);

                u32 pid_col_flags =
                    TALOS_TABLE_COLUMN_FLAG_WIDTH_FIXED | TALOS_TABLE_COLUMN_FLAG_DEFAULT_SORT;

                talos_gui_table_setup_column("PID", pid_col_flags, 50.0f);
                talos_gui_table_setup_column("Name", TALOS_TABLE_COLUMN_FLAG_WIDTH_STRETCH, 0.0f);
                talos_gui_table_setup_column("CPU%", TALOS_TABLE_COLUMN_FLAG_WIDTH_FIXED, 60.0f);
                talos_gui_table_setup_column("MEM", TALOS_TABLE_COLUMN_FLAG_WIDTH_FIXED, 70.0f);
                talos_gui_table_setup_column("State", TALOS_TABLE_COLUMN_FLAG_WIDTH_FIXED, 45.0f);

                talos_gui_table_headers_row();

                bool hotkey_sort_triggered = false;

                if (talos_gui_is_key_pressed(TALOS_KEY_1))  // Sort by PID
                {
                    proc_list->sort_direction =
                        (proc_list->sort == TALOS_SORT_PID)
                            ? (proc_list->sort_direction == TALOS_SORT_DIR_ASCENDING
                                   ? TALOS_SORT_DIR_DESCENDING
                                   : TALOS_SORT_DIR_ASCENDING)
                            : TALOS_SORT_DIR_ASCENDING;

                    proc_list->sort       = TALOS_SORT_PID;
                    hotkey_sort_triggered = true;
                }
                else if (talos_gui_is_key_pressed(TALOS_KEY_2))  // Sort by CPU
                {
                    // Default to Descending for CPU since you want to see heavy processes first!
                    proc_list->sort_direction =
                        (proc_list->sort == TALOS_SORT_CPU)
                            ? (proc_list->sort_direction == TALOS_SORT_DIR_DESCENDING
                                   ? TALOS_SORT_DIR_ASCENDING
                                   : TALOS_SORT_DIR_DESCENDING)
                            : TALOS_SORT_DIR_DESCENDING;

                    proc_list->sort       = TALOS_SORT_CPU;
                    hotkey_sort_triggered = true;
                }
                else if (talos_gui_is_key_pressed(TALOS_KEY_3))  // Sort by MEM
                {
                    proc_list->sort_direction =
                        (proc_list->sort == TALOS_SORT_MEM)
                            ? (proc_list->sort_direction == TALOS_SORT_DIR_DESCENDING
                                   ? TALOS_SORT_DIR_ASCENDING
                                   : TALOS_SORT_DIR_DESCENDING)
                            : TALOS_SORT_DIR_DESCENDING;

                    proc_list->sort       = TALOS_SORT_MEM;
                    hotkey_sort_triggered = true;
                }

                if (hotkey_sort_triggered)
                {
                    if (talos_gui_table_get_sort_specs_dirty())
                    {
                        talos_gui_table_clear_sort_specs_dirty();
                    }
                }

                // mouse click sort
                if (!hotkey_sort_triggered && talos_gui_table_get_sort_specs_dirty())
                {
                    i32 sort_col = talos_gui_table_get_sort_column();
                    i32 sort_dir = talos_gui_table_get_sort_direction();

                    proc_list->sort_dirty = true;

                    if (sort_col == -1)
                    {
                        proc_list->sort           = TALOS_SORT_PID;
                        proc_list->sort_direction = TALOS_SORT_DIR_ASCENDING;
                    }
                    else
                    {
                        talos_sort_mode new_sort = TALOS_SORT_PID;

                        switch (sort_col)
                        {
                            case 0: new_sort = TALOS_SORT_PID; break;
                            case 2: new_sort = TALOS_SORT_CPU; break;
                            case 3: new_sort = TALOS_SORT_MEM; break;
                        }

                        proc_list->sort = new_sort;
                        proc_list->sort_direction =
                            (sort_dir == TALOS_GUI_SORT_DIRECTION_DESCENDING)
                                ? TALOS_SORT_DIR_ASCENDING
                                : TALOS_SORT_DIR_DESCENDING;
                    }

                    talos_gui_table_clear_sort_specs_dirty();
                }

                for (u32 i = 0; i < proc_list->count; i++)
                {
                    talos_process *p      = &proc_list->procs[i];
                    f32            mem_mb = (f32) p->mem_rss_kb / 1024.0f;

                    talos_gui_push_id_int(p->pid);

                    talos_gui_table_next_row();

                    char buf[VX_BUF_SIZE_32];

                    talos_gui_table_set_column_index(0);
                    snprintf(buf, sizeof(buf), "%d", p->pid);

                    bool is_selected = (selected_pid == p->pid);

                    if (talos_gui_selectable(buf, is_selected, 1 << 1))  // span all columns
                    {
                        selected_pid = p->pid;
                        snprintf(selected_proc_name, sizeof(selected_proc_name), "%s", p->name);
                    }

                    talos_gui_table_set_column_index(1);
                    talos_gui_text(p->name);

                    talos_gui_table_set_column_index(2);
                    snprintf(buf, sizeof(buf), "%.1f%%", p->cpu_usage);
                    talos_gui_text(buf);

                    talos_gui_table_set_column_index(3);
                    snprintf(buf, sizeof(buf), "%.1fMB", mem_mb);
                    talos_gui_text(buf);

                    talos_gui_table_set_column_index(4);
                    snprintf(buf, sizeof(buf), "%c", p->state);
                    talos_gui_text(buf);

                    talos_gui_pop_id();
                }

                talos_gui_end_table();
            }
        }

        if (selected_pid != 0 && talos_gui_is_key_pressed(TALOS_KEY_D))
        {
            show_kill_popup = true;
        }

        talos_gui_end_child();
    }
    talos_gui_end();

    // Storage and thermals

    f32 right_column_x = left_margin + card_width + padding;

    // Top right card
    talos_gui_set_next_window_pos(right_column_x, 60.0f);
    talos_gui_set_next_window_size(card_width, half_height);

    if (talos_gui_begin("MemoryCardWrapper", nullptr, card_flags))
    {
        talos_gui_text("Memory Allocation State");
        talos_gui_separator();
        talos_gui_spacing();

        f32 total_gb = (f32) state->mem.total_kb / 1024.0f / 1024.0f;
        f32 used_gb  = (f32) state->mem.used_kb / 1024.0f / 1024.0f;
        f32 avail_gb = (f32) state->mem.available_kb / 1024.0f / 1024.0f;
        f32 cache_gb = (f32) state->mem.cached_kb / 1024.0f / 1024.0f;

        f32 mem_fraction = 0.0f;
        if (state->mem.total_kb > 0)
        {
            mem_fraction = (f32) state->mem.used_kb / (f32) state->mem.total_kb;
        }

        char ram_text[128];
        snprintf(ram_text, sizeof(ram_text), "RAM Usage: %.2f GiB / %.2f GiB", used_gb, total_gb);
        talos_gui_text(ram_text);
        talos_gui_progress_bar(mem_fraction, "");

        talos_gui_spacing();

        snprintf(ram_text, sizeof(ram_text), "Available: %.2f GiB", avail_gb);
        talos_gui_text(ram_text);

        snprintf(ram_text, sizeof(ram_text), "Kernel Cache: %.2f GiB", cache_gb);
        talos_gui_text(ram_text);

        // Render Swap Space if swap allocations are running active on disk
        if (state->mem.swap_total_kb > 0)
        {
            talos_gui_separator();
            talos_gui_spacing();

            f32 swap_total    = (f32) state->mem.swap_total_kb / 1024.0f / 1024.0f;
            f32 swap_used     = (f32) state->mem.swap_used_kb / 1024.0f / 1024.0f;
            f32 swap_fraction = swap_used / swap_total;

            snprintf(ram_text,
                     sizeof(ram_text),
                     "Swap Space: %.2f GiB / %.2f GiB",
                     swap_used,
                     swap_total);
            talos_gui_text(ram_text);
            talos_gui_progress_bar(swap_fraction, "");
        }

        // NOTE: placeholder disk I/O and Network

        talos_gui_separator();
        talos_gui_spacing();

        char io_net_buf[VX_BUF_SIZE_64];
        f32  mid_point_x = card_width * 0.5f;

        talos_gui_begin_group();
        {
            talos_gui_text("Disk Storage I/O");
            talos_gui_spacing();

            snprintf(io_net_buf, sizeof(io_net_buf), "Read:  %.2f MB/s", state->disk.read_mb_sec);
            talos_gui_text(io_net_buf);

            snprintf(io_net_buf, sizeof(io_net_buf), "Write: %.2f MB/s", state->disk.write_mb_sec);
            talos_gui_text(io_net_buf);
        }
        talos_gui_end_group();

        talos_gui_same_line();
        talos_gui_set_cursor_pos_x(mid_point_x + 10.0f);

        talos_gui_begin_group();
        {
            talos_gui_text("Network Traffic");
            talos_gui_spacing();

            snprintf(io_net_buf, sizeof(io_net_buf), "Down: %.1f KB/s", state->net.rx_kb_sec);
            talos_gui_text(io_net_buf);

            snprintf(io_net_buf, sizeof(io_net_buf), "Up:   %.1f KB/s", state->net.tx_kb_sec);
            talos_gui_text(io_net_buf);
        }
        talos_gui_end_group();
    }
    talos_gui_end();

    // Bottom right card
    talos_gui_set_next_window_pos(right_column_x, 60.0f + half_height + padding);
    talos_gui_set_next_window_size(card_width, half_height);

    if (talos_gui_begin("ThermalCardWrapper", nullptr, card_flags))
    {
        talos_gui_text("Thermal Sensor FieldS");
        talos_gui_separator();
        talos_gui_spacing();

        if (state->temps.count == 0)
        {
            talos_gui_text("No hardware monitors reporting data.");
        }
        else
        {
            for (u32 i = 0; i < state->temps.count; ++i)
            {
                talos_temp_sensor *sensor = &state->temps.sensors[i];

                char thermal_label[128];
                snprintf(
                    thermal_label, sizeof(thermal_label), "[%s] %s", sensor->source, sensor->label);
                talos_gui_text(thermal_label);

                talos_gui_same_line();

                char temp_val[32];
                snprintf(temp_val, sizeof(temp_val), "%.1f C", sensor->temp_c);
                talos_gui_text(temp_val);

                if (sensor->temp_crit > 0.0f)
                {
                    f32 thermal_fraction = sensor->temp_c / sensor->temp_crit;
                    if (thermal_fraction > 1.0f)
                    {
                        thermal_fraction = 1.0f;
                    }

                    if (thermal_fraction < 0.0f)
                    {
                        thermal_fraction = 0.0f;
                    }

                    talos_gui_progress_bar(thermal_fraction, "");
                }
                else
                {
                    f32 fallback_fraction = sensor->temp_c / 100.0f;

                    if (fallback_fraction > 1.0f)
                    {
                        fallback_fraction = 1.0f;
                    }
                    talos_gui_progress_bar(fallback_fraction, "");
                }
            }
        }
    }
    talos_gui_end();
    talos_gui_pop_font();

    // Close the master display window frame container context
    talos_gui_end();

    i32 popup_flags = TALOS_GUI_WINDOW_ALWAYS_AUTO_RESIZE;

    if (show_kill_popup)
    {
        talos_gui_open_popup("Kill Process?", 0);
        show_kill_popup = false;
    }

    if (talos_gui_begin_popup_modal("Kill Process?", nullptr, popup_flags))
    {
        char prompt[VX_BUF_SIZE_512];
        snprintf(prompt,
                 sizeof(prompt),
                 "Are you sure you want to terminate %s (PID: %d)?",
                 selected_proc_name,
                 selected_pid);

        talos_gui_text(prompt);
        talos_gui_separator();
        talos_gui_spacing();

        if (talos_gui_is_key_pressed(TALOS_KEY_ESCAPE))
        {
            talos_gui_close_current_popup();
        }

        if (talos_gui_is_key_pressed(TALOS_KEY_ENTER))
        {
            kill(selected_pid, 15);  // Default to clean terminate on Enter
            talos_gui_close_current_popup();
        }

        if (talos_gui_button("Close"))
        {
            kill(selected_pid, 15);

            talos_gui_close_current_popup();
        }

        talos_gui_same_line();

        if (talos_gui_button("Force"))
        {
            kill(selected_pid, 9);

            talos_gui_close_current_popup();
        }

        talos_gui_same_line();

        if (talos_gui_button("Cancel"))
        {
            talos_gui_close_current_popup();
        }

        talos_gui_end_popup();
    }
}
