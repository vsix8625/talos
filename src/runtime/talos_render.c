#include "../gui/talos_gui.h"

#include "globals.h"
#include "config.h"
#include "glad.h"
#include "talos_input.h"
#include "talos_state.h"

#include <SDL3/SDL_timer.h>
#include <inttypes.h>
#include <signal.h>
#include <errno.h>
#include <sys/sysinfo.h>

#define TALOS_MAX_CLUSTERS  8
#define TALOS_MAX_RAW_CORES 256

GLuint g_splash_shader_program_id = 0, g_splash_vao = 0, g_splash_vbo = 0;

static void compile_splash_shader(void);
static u32  shader_compile(const char *source, u32 type);
static u32  shader_link(const char *vert_src, const char *frag_src);

static float proc_history_getter(void *data, int idx)
{
    talos_process *p = (talos_process *) data;

    i32 actual_idx = (p->history_head + idx) % TALOS_HISTORY_COUNT;

    return p->cpu_history[actual_idx];
}

static inline vx_vec4f ui_calculate_load_color(f32 fraction)
{
    vx_vec4f color;

    color.r = fraction * 1.0f;
    color.g = 0.8f * (1.0f - fraction);
    color.b = 0.6f * (1.0f - fraction);
    color.a = 1.0f;

    return color;
}

#define TALOS_IMGUI_STYLE_VAR_ALPHA 0

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

    const f32 transition_speed = 8.0f;
    f32       lerp_factor      = ctx->dt * transition_speed;

    if (lerp_factor > 1.0f)
    {
        lerp_factor = 1.0f;
    }

    static f32 ui_alpha = 0.0f;
    if (ui_alpha < 1.0f)
    {
        ui_alpha += (1.0f - ui_alpha) * lerp_factor;
        if (ui_alpha > 0.99f)
        {
            ui_alpha = 1.0f;
        }
    }

    talos_gui_push_style_var(TALOS_IMGUI_STYLE_VAR_ALPHA, ui_alpha);

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

    const char *fps_mode = "";
    if (ctx->state & TALOS_RUNTIME_STATE_BOOST_FPS)
    {
        fps_mode = "Boosted";
    }
    else if (ctx->state & TALOS_RUNTIME_STATE_LIMIT_FPS)
    {
        fps_mode = "Limited";
    }
    else
    {
        fps_mode = "Normal";
    }

    char header_buf[VX_BUF_SIZE_128];
    snprintf(header_buf,
             sizeof(header_buf),
             "Talos %s | %s | %s Mode | [F1: About]",
             TALOS_VERSION_STRING,
             state->cpu.model,
             fps_mode);
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

    talos_gui_push_font_small();
    if (talos_gui_begin("CPUCardWrapper", nullptr, card_flags))
    {
        static f32 aggregate_visual_load = 0.0f;
        static f32 visual_mhz            = 0.0f;

        f32 aggregate_load     = state->cpu.usage[0];
        f32 aggregate_fraction = aggregate_load / 100.0f;

        if (visual_mhz == 0.0f)
        {
            visual_mhz = (f32) state->cpu.freq_mhz[0];
        }

        struct sysinfo info;

        u64 uptime_secs = 0;
        if (sysinfo(&info) == 0)
        {
            uptime_secs = info.uptime;
        }

        u64 days    = uptime_secs / 86400;
        u64 hours   = (uptime_secs % 86400) / 3600;
        u64 minutes = (uptime_secs % 3600) / 60;
        u64 seconds = uptime_secs % 60;

        visual_mhz += ((f32) state->cpu.freq_mhz[0] - visual_mhz) * lerp_factor;
        char cpu_header_buf[VX_BUF_SIZE_128];

        if (days == 0)
        {
            snprintf(cpu_header_buf,
                     sizeof(cpu_header_buf),
                     "CPU %.0f MHz | %s | Uptime: %02" PRIu64 ":%02" PRIu64 ":%02" PRIu64,
                     visual_mhz,
                     state->cpu.governor,
                     hours,
                     minutes,
                     seconds);
        }
        else
        {
            snprintf(cpu_header_buf,
                     sizeof(cpu_header_buf),
                     "CPU %.0f MHz | Uptime: %" PRIu64 "d %02" PRIu64 ":%02" PRIu64 ":%02" PRIu64,
                     visual_mhz,
                     days,
                     hours,
                     minutes,
                     seconds);
        }

        talos_gui_text(cpu_header_buf);
        talos_gui_separator();
        talos_gui_spacing();

        aggregate_visual_load += (aggregate_fraction - aggregate_visual_load) * lerp_factor;

        char agg_text[VX_BUF_SIZE_64];
        snprintf(
            agg_text, sizeof(agg_text), "Total Workload: %.1f%%", aggregate_visual_load * 100.0f);
        talos_gui_text(agg_text);

        vx_vec4f agg_color = ui_calculate_load_color(aggregate_visual_load);
        talos_gui_push_style_color(TALOS_GUI_COLOR_HISTOGRAM, agg_color);

        talos_gui_progress_bar(aggregate_visual_load, "");

        talos_gui_pop_style_color(1);

        talos_gui_spacing();
        talos_gui_separator();
        talos_gui_spacing();

        if (ctx->state & TALOS_RUNTIME_STATE_CPU_GROUPED)
        {
            u32 group_size = 2;
            if (state->cpu.core_count > 4)
            {
                group_size = (state->cpu.core_count > 32) ? (state->cpu.core_count / 8)
                                                          : (state->cpu.core_count / 4);
            }

            u32 total_groups = state->cpu.core_count / group_size;

            static f32 cluster_visual_load[TALOS_MAX_CLUSTERS];

            for (u32 g = 0; g < total_groups; ++g)
            {
                f32 group_load_sum = 0.0f;
                u32 start_core     = g * group_size;

                for (u32 c = 0; c < group_size; ++c)
                {
                    group_load_sum += state->cpu.usage[(start_core + c) + 1];
                }
                f32 group_avg      = group_load_sum / (f32) group_size;
                f32 group_fraction = group_avg / 100.0f;

                cluster_visual_load[g] += (group_fraction - cluster_visual_load[g]) * lerp_factor;
                f32 current_visual      = cluster_visual_load[g];

                char group_fmt[64];
                snprintf(group_fmt,
                         sizeof(group_fmt),
                         "Cluster %-2u (C%u-C%u)",
                         g,
                         start_core,
                         start_core + group_size - 1);

                talos_gui_text(group_fmt);
                talos_gui_same_line();

                vx_vec4f group_color = ui_calculate_load_color(current_visual);
                talos_gui_push_style_color(TALOS_GUI_COLOR_HISTOGRAM, group_color);

                talos_gui_progress_bar(current_visual, "");

                talos_gui_pop_style_color(1);
            }
        }
        else
        {
            static f32 core_visual_load[TALOS_MAX_RAW_CORES];

            for (u32 i = 0; i < state->cpu.core_count; ++i)
            {
                f32 core_load     = state->cpu.usage[i + 1];
                f32 core_fraction = core_load / 100.0f;

                core_visual_load[i]     += (core_fraction - core_visual_load[i]) * lerp_factor;
                f32 current_core_visual  = core_visual_load[i];

                char core_fmt[32];
                snprintf(core_fmt, sizeof(core_fmt), "Core %-2u", i);
                talos_gui_text(core_fmt);

                talos_gui_same_line();

                vx_vec4f core_color = ui_calculate_load_color(current_core_visual);
                talos_gui_push_style_color(TALOS_GUI_COLOR_HISTOGRAM, core_color);

                talos_gui_progress_bar(current_core_visual, "");

                talos_gui_pop_style_color(1);
            }
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
        char pid_buf[VX_BUF_SIZE_64];
        snprintf(pid_buf, sizeof(pid_buf), "PID | Tasks: %u", proc_list->count);
        talos_gui_text(pid_buf);
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
                talos_gui_table_setup_column("CPU%", TALOS_TABLE_COLUMN_FLAG_WIDTH_FIXED, 70.0f);
                talos_gui_table_setup_column("MEM", TALOS_TABLE_COLUMN_FLAG_WIDTH_FIXED, 140.0f);
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

                talos_gui_list_clipper clipper = {0};
                talos_gui_list_clipper_begin(&clipper, (i32) proc_list->count);
                while (talos_gui_list_clipper_step(&clipper))
                {
                    for (i32 i = talos_gui_list_clipper_display_start(&clipper);
                         i < talos_gui_list_clipper_display_end(&clipper);
                         i++)
                    {
                        talos_process *p      = &proc_list->procs[i];
                        f32            mem_mb = (f32) p->mem_rss_kb / 1024.0f;

                        talos_gui_push_id_int(p->pid);

                        talos_gui_table_next_row();

                        char buf[VX_BUF_SIZE_32];

                        talos_gui_table_set_column_index(0);
                        snprintf(buf, sizeof(buf), "%d", p->pid);

                        bool is_selected = (selected_pid == p->pid);

                        vx_vec4f pid_text_color;
                        bool     apply_text_color = true;

                        if (p->state == 'R')
                        {
                            pid_text_color =
                                (vx_vec4f) {.r = 0.2f, .g = 1.0f, .b = 0.2f, .a = 1.0f};
                        }
                        else if (p->state == 'D')
                        {
                            pid_text_color =
                                (vx_vec4f) {.r = 1.0f, .g = 0.2f, .b = 0.2f, .a = 1.0f};
                        }
                        else if (p->state == 'Z')
                        {
                            pid_text_color =
                                (vx_vec4f) {.r = 0.7f, .g = 0.4f, .b = 0.8f, .a = 1.0f};
                        }
                        else
                        {
                            apply_text_color = false;
                        }

                        if (apply_text_color)
                        {
                            talos_gui_push_style_color(TALOS_GUI_COLOR_TEXT, pid_text_color);
                        }

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

                        if (apply_text_color)
                        {
                            talos_gui_pop_style_color(1);
                        }

                        //----------------------------------------------------------------------------------------------------
                        // telemetry window

                        if (is_selected)
                        {
                            f32 window_w = (f32) ctx->width * 0.55f;
                            f32 window_h = (f32) ctx->height * 0.45f;

                            if (window_w < 520.0f)
                            {
                                window_w = 520.0f;
                            }

                            if (window_h < 320.0f)
                            {
                                window_h = 320.0f;
                            }

                            f32 pos_x = ((f32) ctx->width - window_w) * 0.5f;
                            f32 pos_y = ((f32) ctx->height - window_h) * 0.5f;

                            talos_gui_set_next_window_pos(pos_x, pos_y);
                            talos_gui_set_next_window_size(window_w, window_h);

                            char title_buf[VX_BUF_SIZE_128];
                            snprintf(title_buf,
                                     sizeof(title_buf),
                                     "Telemetry Profile: %s (PID: %d)##popup",
                                     p->name,
                                     p->pid);

                            if (talos_gui_begin(title_buf,
                                                nullptr,
                                                TALOS_GUI_WINDOW_NO_COLLAPSE |
                                                    TALOS_GUI_WINDOW_NO_RESIZE))
                            {
                                char meta_buf[VX_BUF_SIZE_128];

                                talos_gui_text("WORKLOAD HISTORY");
                                talos_gui_separator();

                                snprintf(meta_buf, sizeof(meta_buf), "Name:  %s", p->name);
                                talos_gui_text(meta_buf);

                                snprintf(meta_buf,
                                         sizeof(meta_buf),
                                         "State: %c  |  Memory: %.1fMB",
                                         p->state,
                                         mem_mb);
                                talos_gui_text(meta_buf);

                                talos_gui_spacing();
                                talos_gui_separator();
                                talos_gui_spacing();

                                char graph_overlay[VX_BUF_SIZE_32];
                                snprintf(graph_overlay,
                                         sizeof(graph_overlay),
                                         "Live: %.1f%%",
                                         p->cpu_usage);

                                f32 graph_w = window_w - 80.0f;
                                f32 graph_h = window_h - 300.0f;

                                talos_gui_plot_lines("##cpu_profile_graph",
                                                     proc_history_getter,
                                                     p,
                                                     TALOS_HISTORY_COUNT,
                                                     graph_overlay,
                                                     0.0f,
                                                     80.0f,
                                                     graph_w,
                                                     graph_h);

                                talos_gui_spacing();
                                talos_gui_separator();
                                talos_gui_spacing();

                                if (talos_gui_button("Close") ||
                                    talos_gui_is_key_pressed(TALOS_KEY_ESCAPE))
                                {
                                    selected_pid = 0;
                                }

                                talos_gui_end();
                            }
                        }

                        //----------------------------------------------------------------------------------------------------

                        talos_gui_pop_id();
                    }
                }
                talos_gui_list_clipper_end(&clipper);

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
        talos_gui_text("RAM & I/O");
        talos_gui_separator();
        talos_gui_spacing();

        static f32 ram_visual_load = 0.0f;

        f32 total_gb = (f32) state->mem.total_kb / 1024.0f / 1024.0f;
        f32 used_gb  = (f32) state->mem.used_kb / 1024.0f / 1024.0f;
        f32 avail_gb = (f32) state->mem.available_kb / 1024.0f / 1024.0f;
        f32 cache_gb = (f32) state->mem.cached_kb / 1024.0f / 1024.0f;

        f32 ram_fraction = 0.0f;
        if (state->mem.total_kb > 0)
        {
            ram_fraction = (f32) state->mem.used_kb / (f32) state->mem.total_kb;
        }

        ram_visual_load += (ram_fraction - ram_visual_load) * lerp_factor;

        char ram_text[128];
        snprintf(ram_text, sizeof(ram_text), "RAM Usage: %.2f GiB / %.2f GiB", used_gb, total_gb);
        talos_gui_text(ram_text);

        vx_vec4f ram_color = ui_calculate_load_color(ram_visual_load);
        talos_gui_push_style_color(TALOS_GUI_COLOR_HISTOGRAM, ram_color);
        talos_gui_progress_bar(ram_visual_load, "");
        talos_gui_pop_style_color(1);

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

            static f32 swap_visual_load = 0.0f;

            f32 swap_total    = (f32) state->mem.swap_total_kb / 1024.0f / 1024.0f;
            f32 swap_used     = (f32) state->mem.swap_used_kb / 1024.0f / 1024.0f;
            f32 swap_fraction = swap_used / swap_total;

            swap_visual_load += (swap_fraction - swap_visual_load) * lerp_factor;

            snprintf(ram_text,
                     sizeof(ram_text),
                     "Swap Space: %.2f GiB / %.2f GiB",
                     swap_used,
                     swap_total);
            talos_gui_text(ram_text);

            vx_vec4f swap_color = ui_calculate_load_color(swap_visual_load);
            talos_gui_push_style_color(TALOS_GUI_COLOR_HISTOGRAM, swap_color);
            talos_gui_progress_bar(swap_visual_load, "");
            talos_gui_pop_style_color(1);
        }

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
    f32 power_card_height   = 70.0f;
    f32 thermal_card_height = half_height - power_card_height - padding;

    talos_gui_set_next_window_pos(right_column_x, 60.0f + half_height + padding);
    talos_gui_set_next_window_size(card_width, thermal_card_height);

    if (talos_gui_begin("ThermalCardWrapper", nullptr, card_flags))
    {
        talos_gui_text("FAN");
        talos_gui_separator();
        talos_gui_spacing();

        if (state->temps.count == 0)
        {
            talos_gui_text("No hardware monitors reporting data.");
        }
        else
        {
            // hardcode 8 for now
            static f32 temp_visual_load[8] = {0};

            u32 active_sensors = state->temps.count;
            if (active_sensors > 8)
            {
                active_sensors = 8;
            }

            for (u32 i = 0; i < state->temps.count; ++i)
            {
                talos_temp_sensor *sensor = &state->temps.sensors[i];

                if (sensor->is_frozen)
                {
                    continue;
                }

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
                    f32 raw_fraction = 0.0f;
                    if (sensor->temp_crit > 0.0f)
                    {
                        raw_fraction = sensor->temp_c / sensor->temp_crit;
                    }
                    else
                    {
                        raw_fraction = sensor->temp_c / 100.0f;
                    }

                    if (raw_fraction > 1.0f)
                    {
                        raw_fraction = 1.0f;
                    }
                    if (raw_fraction < 0.0f)
                    {
                        raw_fraction = 0.0f;
                    }

                    temp_visual_load[i] += (raw_fraction - temp_visual_load[i]) * lerp_factor;
                    f32 current_visual   = temp_visual_load[i];

                    vx_vec4f sensor_color = ui_calculate_load_color(current_visual);
                    talos_gui_push_style_color(TALOS_GUI_COLOR_HISTOGRAM, sensor_color);

                    talos_gui_progress_bar(current_visual, "");

                    talos_gui_pop_style_color(1);
                }
                else
                {
                    f32 fallback_fraction = sensor->temp_c / 100.0f;

                    if (fallback_fraction > 1.0f)
                    {
                        fallback_fraction = 1.0f;
                    }

                    vx_vec4f sensor_color = ui_calculate_load_color(fallback_fraction);
                    talos_gui_push_style_color(TALOS_GUI_COLOR_HISTOGRAM, sensor_color);

                    talos_gui_progress_bar(fallback_fraction, "");

                    talos_gui_pop_style_color(1);
                }
            }
        }
    }
    talos_gui_end();
    talos_gui_pop_font();

    // POWER CONTROL CARD
    f32 power_card_y = 60.0f + half_height + padding + thermal_card_height + padding;

    talos_gui_set_next_window_pos(right_column_x, power_card_y);
    talos_gui_set_next_window_size(card_width, power_card_height);

    static bool show_shutdown_confirm = false;
    static bool show_reboot_confirm   = false;

    if (talos_gui_begin("PowerControlCardWrapper", nullptr, card_flags))
    {
        f32 button_width = (card_width - 15.0f) * 0.5f;

        vx_vec4f red_button = {.r = 0.75f, .g = 0.15f, .b = 0.15f, .a = 1.0f};
        vx_vec4f red_hover  = {.r = 0.90f, .g = 0.20f, .b = 0.20f, .a = 1.0f};

        talos_gui_push_style_color(TALOS_GUI_COLOR_BUTTON, red_button);
        talos_gui_push_style_color(TALOS_GUI_COLOR_BUTTON_HOVERED, red_hover);

        if (talos_gui_button_xy("Shutdown", button_width, 0.0f))
        {
            show_shutdown_confirm = true;
        }

        talos_gui_pop_style_color(2);

        talos_gui_same_line();

        if (talos_gui_button_xy("Reboot", button_width, 0.0f))
        {
            show_reboot_confirm = true;
        }
    }
    talos_gui_end();  // power control card end

    // Close the master display window frame container context
    talos_gui_end();

    // CONTEXT POPUP CONFIRMATIONS
    if (show_shutdown_confirm)
    {
        talos_gui_open_popup("Confirm Shutdown", 0);
    }

    if (show_reboot_confirm)
    {
        talos_gui_open_popup("Confirm Reboot", 0);
    }

    if (talos_gui_begin_popup_modal(
            "Confirm Shutdown", nullptr, TALOS_GUI_WINDOW_ALWAYS_AUTO_RESIZE))
    {
        talos_gui_text("Are you sure you want to turn off the machine?");
        talos_gui_spacing();

        if (talos_gui_button_xy("Yes", 120.0f, 0.0f))
        {
            talos_system_shutdown();
            show_shutdown_confirm = false;
            talos_gui_close_current_popup();
        }

        talos_gui_same_line();

        if (talos_gui_button_xy("Cancel", 120.0f, 0.0f))
        {
            show_shutdown_confirm = false;
            talos_gui_close_current_popup();
        }
        talos_gui_end_popup();
    }

    if (talos_gui_begin_popup_modal("Confirm Reboot", nullptr, TALOS_GUI_WINDOW_ALWAYS_AUTO_RESIZE))
    {
        talos_gui_text("Are you sure you want to restart the machine?");
        talos_gui_spacing();

        if (talos_gui_button_xy("Yes", 120.0f, 0.0f))
        {
            talos_system_reboot();
            show_reboot_confirm = false;
            talos_gui_close_current_popup();
        }

        talos_gui_same_line();

        if (talos_gui_button_xy("Cancel", 120.0f, 0.0f))
        {
            show_reboot_confirm = false;
            talos_gui_close_current_popup();
        }
        talos_gui_end_popup();
    }

    i32 popup_flags = TALOS_GUI_WINDOW_ALWAYS_AUTO_RESIZE;

    if (show_kill_popup)
    {
        talos_gui_open_popup("Kill Process?", 0);
        show_kill_popup = false;
    }

    static i32  last_kill_errno = 0;
    static bool kill_failed     = false;

    if (talos_gui_begin_popup_modal("Kill Process?", nullptr, popup_flags))
    {
        char prompt[VX_BUF_SIZE_512];
        snprintf(prompt,
                 sizeof(prompt),
                 "Are you sure you want to terminate %s (PID: %d)?",
                 selected_proc_name,
                 selected_pid);
        talos_gui_text(prompt);

        if (kill_failed)
        {
            talos_gui_spacing();

            talos_gui_push_style_color(TALOS_GUI_COLOR_TEXT,
                                       (vx_vec4f) {.r = 1.0f, .g = 0.2f, .b = 0.2f, .a = 1.0f});

            if (last_kill_errno == EPERM)
            {
                talos_gui_text("[KERNEL ERROR: EPERM - Operation Not Permitted (Requires Root)]");
            }
            else
            {
                char err_buf[VX_BUF_SIZE_128];
                snprintf(err_buf, sizeof(err_buf), "[KERNEL ERROR: %s]", strerror(last_kill_errno));
                talos_gui_text(err_buf);
            }

            talos_gui_pop_style_color(1);
        }

        talos_gui_separator();
        talos_gui_spacing();

        if (talos_gui_is_key_pressed(TALOS_KEY_ESCAPE))
        {
            kill_failed = false;
            talos_gui_close_current_popup();
        }

        if (talos_gui_is_key_pressed(TALOS_KEY_ENTER))
        {
            if (kill(selected_pid, 15) == -1)
            {
                last_kill_errno = errno;
                kill_failed     = true;
            }
            else
            {
                kill_failed = false;
                talos_gui_close_current_popup();
            }
        }

        if (talos_gui_button("Terminate"))
        {
            if (kill(selected_pid, 15) == -1)
            {
                last_kill_errno = errno;
                kill_failed     = true;
            }
            else
            {
                kill_failed = false;
                talos_gui_close_current_popup();
            }
        }

        talos_gui_same_line();

        if (talos_gui_button("Force"))
        {
            if (kill(selected_pid, 9) == -1)
            {
                last_kill_errno = errno;
                kill_failed     = true;
            }
            else
            {
                kill_failed = false;
                talos_gui_close_current_popup();
            }
        }

        talos_gui_same_line();

        if (talos_gui_button("Cancel"))
        {
            kill_failed = false;
            talos_gui_close_current_popup();
        }

        talos_gui_end_popup();
    }

    talos_gui_pop_style_var(1);
}

// DASHBOARD END

void talos_ui_render_about_popup(struct talos_ctx *ctx)
{
    if (ctx == nullptr)
    {
        return;
    }

    f32 window_w = (f32) ctx->width * 0.55f;
    f32 window_h = (f32) ctx->height * 0.65f;

    if (window_w < 500.0f)
    {
        window_w = 500.0f;
    }

    if (window_h < 420.0f)
    {
        window_h = 420.0f;
    }

    f32 pos_x = ((f32) ctx->width - window_w) * 0.5f;
    f32 pos_y = ((f32) ctx->height - window_h) * 0.5f;

    talos_gui_set_next_window_pos(pos_x, pos_y);
    talos_gui_set_next_window_size(window_w, window_h);

    if (talos_gui_begin(
            "About Talos", nullptr, TALOS_GUI_WINDOW_NO_COLLAPSE | TALOS_GUI_WINDOW_NO_RESIZE))
    {
        talos_gui_text("TALOS SYSTEM MONITOR");
        talos_gui_text_disabled("A native Linux monitor.");

        talos_gui_separator();
        talos_gui_spacing();

        talos_gui_text("Author:  vsix8625");
        talos_gui_text_link("GitHub:      https://github.com/vsix8625",
                            "https://github.com/vsix8625");
        talos_gui_text_link("Project:     https://github.com/vsix8625/talos",
                            "https://github.com/vsix8625/talos");
        talos_gui_text_link("Foundation:  https://github.com/vsix86/vx",
                            "https://github.com/vsix86/vx");
        talos_gui_text_link("Tool:        https://github.com/vsix8625/storm-knell",
                            "https://github.com/vsix8625/storm-knell");

        talos_gui_spacing();
        talos_gui_separator();
        talos_gui_spacing();

        talos_gui_text_disabled("Upstream Open-Source Libraries:");
        talos_gui_text_link("Windowing:   SDL3 (https://github.com/libsdl-org/SDL)",
                            "https://github.com/libsdl-org/SDL");
        talos_gui_text_link("Interface:   Dear ImGui (https://github.com/ocornut/imgui)",
                            "https://github.com/ocornut/imgui");
        talos_gui_text_link("GL Loader:   GLAD (https://github.com/Dav1dde/glad)",
                            "https://github.com/Dav1dde/glad");
        talos_gui_text_link("Graphics:    OpenGL Core Profile (https://www.opengl.org)",
                            "https://www.opengl.org");
        talos_gui_text_link(
            "Typography:  JetBrains Mono Nerd Font (https://github.com/ryanoasis/nerd-fonts)",
            "https://github.com/ryanoasis/nerd-fonts");

        talos_gui_spacing();
        talos_gui_separator();
        talos_gui_spacing();

        talos_gui_text_disabled("Operational Hotkeys:");
        talos_gui_text("[1, 2, 3] Sort processes via (PID, CPU, Memory usage)");
        talos_gui_text("[g]       Change CPU core layout view");
        talos_gui_text("[d]       Kill or Force Quit the selected process");
        talos_gui_text("[q]       Exit Talos");
        talos_gui_text("[F1]      Open About window");
        talos_gui_text("[F2]      Toggle Limited/Normal modes");
        talos_gui_text("[F3]      Toggle Boosted/Normal modes");
        talos_gui_text("[F4]      Cycle Fan modes (if installed)");
        talos_gui_text("[F5]      Cycle Governor modes (if installed)");
        talos_gui_text("[F11]     Toggle Fullscreen mode");
        talos_gui_text("[F12]     Exit Talos");

        talos_gui_spacing();
        talos_gui_separator();
        talos_gui_spacing();

        if (talos_gui_button("Close"))
        {
            ctx->state &= ~TALOS_RUNTIME_STATE_ABOUT_WINDOW;
        }

        talos_gui_end();
    }
}

void talos_render_stage_dashboard(struct talos_ctx *ctx, void *data)
{
    if (ctx == nullptr || data == nullptr)
    {
        return;
    }

    talos_state *cpu_state = (talos_state *) data;

    u32 ui_idx = atomic_load_explicit(&cpu_state->proc_state.active_idx, memory_order_acquire);
    talos_proc_list *ui_list = &cpu_state->proc_state.buffers[ui_idx];

    talos_ui_render_dashboard(ctx, cpu_state, ui_list, ctx->width, ctx->height);
}

typedef struct
{
    vx_vec2f position;  // x, y (v_pos)
    vx_vec4f color;     // r, g, b, a (v_color)
} talos_vertext;

talos_vertext splash_vertices[] = {
    {{-1.0f, -1.0f}, {.r = 0.8f, .g = 0.4f, .b = 0.1f, .a = 1.0f}},  // Bottom Left
    {{1.0f, -1.0f}, {.r = 0.8f, .g = 0.4f, .b = 0.1f, .a = 1.0f}},   // Bottom Right
    {{1.0f, 1.0f}, {.r = 0.9f, .g = 0.6f, .b = 0.2f, .a = 1.0f}},    // Top Right

    {{-1.0f, -1.0f}, {.r = 0.8f, .g = 0.4f, .b = 0.1f, .a = 1.0f}},  // Bottom Left
    {{1.0f, 1.0f}, {.r = 0.9f, .g = 0.6f, .b = 0.2f, .a = 1.0f}},    // Top Right
    {{-1.0f, 1.0f}, {.r = 0.9f, .g = 0.6f, .b = 0.2f, .a = 1.0f}}    // Top Left
};

const char g_splash_shader_src[] = {
#embed "../../assets/shaders/splash.frag"
    ,
    0};

void talos_init_splash_geometry(void)
{
    compile_splash_shader();

    glGenVertexArrays(1, &g_splash_vao);
    glBindVertexArray(g_splash_vao);

    glGenBuffers(1, &g_splash_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_splash_vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(splash_vertices), splash_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(talos_vertext),
                          (void *) offsetof(talos_vertext, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 4, GL_FLOAT, GL_FALSE, sizeof(talos_vertext), (void *) offsetof(talos_vertext, color));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

const char *splash_vert_src =
    "#version 450 core\n"
    "layout (location = 0) in vec2 a_pos;\n"
    "layout (location = 1) in vec4 a_color;\n"
    "\n"
    "uniform vec2 u_aspect_ratio;\n"
    "uniform float u_scale;\n"
    "\n"
    "out vec2 v_pos;\n"
    "out vec4 v_color;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    v_pos = a_pos;\n"
    "    v_color = a_color;\n"
    "    \n"
    "    // Scale the position and adjust for screen aspect ratio stretch\n"
    "    vec2 scaled_pos = a_pos * u_scale * u_aspect_ratio;\n"
    "    gl_Position = vec4(scaled_pos, 0.0, 1.0);\n"
    "}\n";

void talos_render_stage_splash(struct talos_ctx *ctx, void *data)
{
    if (ctx == nullptr || data == nullptr)
    {
        return;
    }

    f32 *timer = (f32 *) data;

    *timer += ctx->dt;

    const f32 MAX_SPLASH_TIME = 1.8f;

    f32 progress = *timer / MAX_SPLASH_TIME;
    if (progress > 1.0f)
    {
        progress = 1.0f;
    }

    glUseProgram(g_splash_shader_program_id);

    f32 aspect_x = 1.0f;
    f32 aspect_y = 1.0f;

    if (ctx->width > ctx->height)
    {
        aspect_x = (f32) ctx->height / (f32) ctx->width;
    }
    else
    {
        aspect_y = (f32) ctx->width / (f32) ctx->height;
    }

    glUniform2f(
        glGetUniformLocation(g_splash_shader_program_id, "u_aspect_ratio"), aspect_x, aspect_y);
    glUniform1f(glGetUniformLocation(g_splash_shader_program_id, "u_scale"), 0.20f);

    glUniform1f(glGetUniformLocation(g_splash_shader_program_id, "u_progress"), progress);
    glUniform3f(glGetUniformLocation(g_splash_shader_program_id, "u_light_dir"), -0.5f, 0.8f, 1.0f);

    glBindVertexArray(g_splash_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    if (*timer >= MAX_SPLASH_TIME)
    {
        ctx->state &= ~TALOS_RUNTIME_STATE_SPLASH;
    }
}

static u32 shader_compile(const char *source, u32 type)
{
    if (source == nullptr || source[0] == '\0')
    {
        return 0;
    }

    u32 out_shader = glCreateShader(type);

    glShaderSource(out_shader, 1, &source, nullptr);
    glCompileShader(out_shader);

    i32 compiled;
    glGetShaderiv(out_shader, GL_COMPILE_STATUS, &compiled);

    if (compiled != GL_TRUE)
    {
        i32  log_len = 0;
        char msg[1024];
        glGetShaderInfoLog(out_shader, 1024, &log_len, msg);
        vx_errlog("Shader compile error: %s", msg);
        return 0;
    }

    return out_shader;
}

static u32 shader_link(const char *vert_src, const char *frag_src)
{
    u32 vert_shader = shader_compile(vert_src, GL_VERTEX_SHADER);
    u32 frag_shader = shader_compile(frag_src, GL_FRAGMENT_SHADER);

    if (vert_shader == 0 || frag_shader == 0)
    {
        vx_errlog("Failed to compile shaders");
        return 0;
    }

    u32 prog = glCreateProgram();
    glAttachShader(prog, vert_shader);
    glAttachShader(prog, frag_shader);
    glLinkProgram(prog);

    i32 prog_linked;
    glGetProgramiv(prog, GL_LINK_STATUS, &prog_linked);

    if (prog_linked != GL_TRUE)
    {
        i32  log_len = 0;
        char msg[1024];
        glGetProgramInfoLog(prog, 1024, &log_len, msg);
        vx_errlog("Failed to link shaders: %s", msg);
        glDeleteProgram(prog);
        prog = 0;
    }

    glDetachShader(prog, vert_shader);
    glDetachShader(prog, frag_shader);
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    vx_dbglog("Shader linked");
    return prog;
}

static void compile_splash_shader(void)
{
    g_splash_shader_program_id = shader_link(splash_vert_src, g_splash_shader_src);

    if (g_splash_shader_program_id == 0)
    {
        return;
    }

    glGenVertexArrays(1, &g_splash_vao);
    glBindVertexArray(g_splash_vao);

    glGenBuffers(1, &g_splash_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_splash_vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(splash_vertices), splash_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(talos_vertext),
                          (void *) offsetof(talos_vertext, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 4, GL_FLOAT, GL_FALSE, sizeof(talos_vertext), (void *) offsetof(talos_vertext, color));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void talos_destroy_splash_geometry(void)
{
    if (g_splash_vbo)
    {
        glDeleteBuffers(1, &g_splash_vbo);
    }
    if (g_splash_vao)
    {
        glDeleteVertexArrays(1, &g_splash_vao);
    }
    if (g_splash_shader_program_id)
    {
        glDeleteProgram(g_splash_shader_program_id);
    }
}
