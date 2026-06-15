#include "talos_gui.h"
#include "glad.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

extern "C"
{
#include "../../external/vx/include/vx.h"
}

using namespace ImGui;

static ImFont *g_imgui_font_small = nullptr;
static ImFont *g_imgui_font_large = nullptr;

TALOS_API int talos_gui_init(void *window, void *gl_ctx)
{
    IMGUI_CHECKVERSION();
    CreateContext();

    if (!ImGui_ImplSDL3_InitForOpenGL((SDL_Window *) window, gl_ctx))
    {
        return 0;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 450"))
    {
        return 0;
    }

    ImGuiStyle &style      = GetStyle();
    style.WindowRounding   = 12.0f;
    style.FrameRounding    = 6.0f;
    style.ChildRounding    = 8.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize  = 1.0f;
    style.WindowPadding    = ImVec2(16.0f, 16.0f);
    style.ItemSpacing      = ImVec2(12.0f, 10.0f);

    ImVec4 *colors            = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.07f, 0.10f, 0.95f);  // Matte deep canvas
    colors[ImGuiCol_Border]   = ImVec4(0.15f, 0.18f, 0.25f, 1.00f);  // Low-opacity glass edge
    colors[ImGuiCol_FrameBg]  = ImVec4(0.12f, 0.14f, 0.18f, 1.00f);  // Unfilled track background
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 0.75f, 0.60f, 1.00f);  // Electric Teal
    colors[ImGuiCol_Text]          = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);  // Crisp white
    colors[ImGuiCol_TextDisabled]  = ImVec4(0.48f, 0.52f, 0.64f, 1.00f);  // Darker gray labels

    ImGuiIO &io    = GetIO();
    io.IniFilename = nullptr;

    const char *home = vx_platform_get_home_dir();

    char user_font_dir[VX_BUF_SIZE_1024]  = {0};
    char user_font_path[VX_BUF_SIZE_4096] = {0};

    if (home)
    {
        snprintf(user_font_dir, sizeof(user_font_dir), "%s/.local/share/talos/assets/fonts", home);

        snprintf(user_font_path,
                 sizeof(user_font_path),
                 "%s/JetBrainsMonoNerdFont-Medium.ttf",
                 user_font_dir);
    }

    const char *font_search_paths[] = {
        user_font_path,                                   // Local user data directory
        "assets/fonts/JetBrainsMonoNerdFont-Medium.ttf",  // Portable development relative path
        "/usr/share/fonts/TTF/JetBrainsMonoNerdFont-Medium.ttf",  // Standard Linux system path
        "/usr/share/fonts/noto/NotoSans-Regular.ttf"              // Total generic system fallback
    };

    const char *resolved_path = nullptr;

    u32 path_count = sizeof(font_search_paths) / sizeof(font_search_paths[0]);

    for (u32 i = 0; i < path_count; ++i)
    {
        if (font_search_paths[i] && font_search_paths[i][0] != '\0')
        {
            if (vx_isfile(font_search_paths[i]))
            {
                resolved_path = font_search_paths[i];
                break;
            }
        }
    }

    if (resolved_path && (resolved_path != user_font_path) && home)
    {
        if (vx_mkdir_p(user_font_dir) != VX_OK)
        {
            VX_ASSERT_LOG("mkdir failed");
            return 0;
        }

        if (vx_fs_cp(resolved_path, user_font_path))
        {
            vx_dbglog("Successfully cached font asset to local storage: %s", user_font_path);
            resolved_path = user_font_path;
        }
        else
        {
            vx_dbglog("Failed local asset storage write verification pass.");
        }
    }

    if (resolved_path)
    {
        g_imgui_font_small = io.Fonts->AddFontFromFileTTF(resolved_path, 20.0f);
        g_imgui_font_large = io.Fonts->AddFontFromFileTTF(resolved_path, 28.0f);

        vx_dbglog("Loaded font from: %s", resolved_path);
    }
    return 1;
}

TALOS_API void talos_gui_shutdown(void)
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    DestroyContext();
}

TALOS_API void talos_gui_new_frame(void)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    NewFrame();
}

TALOS_API void talos_gui_render(void)
{
    Render();
    ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());
}

TALOS_API int talos_gui_begin(const char *title, int *open, int flags)
{
    return Begin(title, open ? (bool *) open : nullptr, (ImGuiWindowFlags) flags);
}

TALOS_API void talos_gui_end(void)
{
    End();
}

TALOS_API void talos_gui_text(const char *text)
{
    TextUnformatted(text);
}

TALOS_API int talos_gui_button(const char *label)
{
    return Button(label);
}

TALOS_API int talos_gui_checkbox(const char *label, int *v)
{
    bool b = (bool) *v;
    int  r = Checkbox(label, &b);
    *v     = (int) b;
    return r;
}

TALOS_API int talos_gui_slider_float(const char *label, float *v, float min, float max)
{
    return SliderFloat(label, v, min, max);
}

TALOS_API int talos_gui_input_text(const char *label, char *buf, size_t buf_size)
{
    return InputText(label, buf, buf_size);
}

TALOS_API void talos_gui_separator(void)
{
    Separator();
}

TALOS_API void talos_gui_same_line(void)
{
    SameLine();
}

TALOS_API void talos_gui_spacing(void)
{
    Spacing();
}

TALOS_API void talos_gui_set_next_window_pos(float x, float y)
{
    SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
}

TALOS_API void talos_gui_set_next_window_size(float w, float h)
{
    SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
}

TALOS_API void talos_gui_set_next_window_bg_alpha(float alpha)
{
    SetNextWindowBgAlpha(alpha);
}

TALOS_API void talos_gui_progress_bar(float fraction, const char *overlay)
{
    ProgressBar(fraction, ImVec2(-1.0f, 0.0f), overlay);
}

TALOS_API float talos_gui_get_framerate(void)
{
    return GetIO().Framerate;
}

TALOS_API void talos_gui_begin_frame(void)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    NewFrame();
}

TALOS_API void talos_gui_render_frame(void)
{
    Render();

    ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());
}

TALOS_API int talos_gui_begin_child(const char *str_id, float size_x, float size_y, int border)
{
    return BeginChild(str_id, ImVec2(size_x, size_y), border ? ImGuiChildFlags_Borders : 0);
}

TALOS_API void talos_gui_end_child(void)
{
    EndChild();
}

TALOS_API void talos_gui_push_font_large(void)
{
    if (g_imgui_font_large)
    {
        PushFont(g_imgui_font_large);
    }
}

TALOS_API void talos_gui_push_font_small(void)
{
    if (g_imgui_font_small)
    {
        PushFont(g_imgui_font_small);
    }
}

TALOS_API void talos_gui_pop_font(void)
{
    PopFont();
}

TALOS_API void talos_gui_push_style_var_float2(int idx, f32 x, f32 y)
{
    PushStyleVar(idx, ImVec2(x, y));
}

TALOS_API void talos_gui_pop_style_var(int count)
{
    PopStyleVar(count);
}

TALOS_API int talos_gui_begin_table(const char *id, int columns, int flags, float outer_size_y)
{
    ImVec2 outer_size = ImVec2(0.0f, outer_size_y);

    if (flags & ImGuiTableFlags_ScrollY)
    {
        outer_size.x = GetContentRegionAvail().x;
    }

    return BeginTable(id, columns, (ImGuiTableFlags) flags, outer_size);
}

TALOS_API void talos_gui_end_table(void)
{
    EndTable();
}

TALOS_API void talos_gui_table_next_row(void)
{
    TableNextRow();
}

TALOS_API int talos_gui_table_set_column_index(int col)
{
    return TableSetColumnIndex(col);
}

TALOS_API void talos_gui_table_setup_column(const char *label, int flags, float width)
{
    TableSetupColumn(label, (ImGuiTableColumnFlags) flags, width);
}

TALOS_API void talos_gui_table_setup_scroll_freeze(int cols, int rows)
{
    TableSetupScrollFreeze(cols, rows);
}

TALOS_API void talos_gui_table_headers_row(void)
{
    TableHeadersRow();
}

TALOS_API int talos_gui_event_process(void *event)
{
    return ImGui_ImplSDL3_ProcessEvent(static_cast<SDL_Event *>(event));
}

TALOS_API float talos_gui_get_scroll_y(void)
{
    return GetScrollY();
}

TALOS_API void talos_gui_set_scroll_y(float y)
{
    SetScrollY(y);
}

TALOS_API void talos_gui_push_id_int(int int_id)
{
    PushID(int_id);
}

TALOS_API void talos_gui_pop_id(void)
{
    PopID();
}

// --- Selectables & Interaction ---

TALOS_API bool talos_gui_selectable(const char *label, bool selected, int flags)
{
    // Using SpanAllColumns lets the click target cover the whole row
    return Selectable(label, selected, flags);
}

TALOS_API bool talos_gui_is_item_clicked(int mouse_button)
{
    return IsItemClicked(mouse_button);
}

// --- Popup & Modal Management ---

TALOS_API void talos_gui_open_popup(const char *str_id, int popup_flags)
{
    OpenPopup(str_id, popup_flags);
}

TALOS_API bool talos_gui_begin_popup_modal(const char *name, bool *p_open, int flags)
{
    return BeginPopupModal(name, p_open, flags);
}

TALOS_API void talos_gui_end_popup(void)
{
    EndPopup();
}

TALOS_API void talos_gui_close_current_popup(void)
{
    CloseCurrentPopup();
}

TALOS_API bool talos_gui_table_get_sort_specs_dirty(void)
{
    ImGuiTableSortSpecs *specs = TableGetSortSpecs();
    return (specs != nullptr) ? specs->SpecsDirty : false;
}

TALOS_API void talos_gui_table_clear_sort_specs_dirty(void)
{
    ImGuiTableSortSpecs *specs = TableGetSortSpecs();
    if (specs)
    {
        specs->SpecsDirty = false;
    }
}

TALOS_API int talos_gui_table_get_sort_column(void)
{
    ImGuiTableSortSpecs *specs = TableGetSortSpecs();
    if (specs && specs->SpecsCount > 0)
    {
        return (int) specs->Specs[0].ColumnIndex;
    }
    return -1;
}

TALOS_API int talos_gui_table_get_sort_direction(void)
{
    ImGuiTableSortSpecs *specs = TableGetSortSpecs();
    if (specs && specs->SpecsCount > 0)
    {
        switch (specs->Specs[0].SortDirection)
        {
            case ImGuiSortDirection_Ascending: return TALOS_GUI_SORT_DIRECTION_ASCENDING;
            case ImGuiSortDirection_Descending: return TALOS_GUI_SORT_DIRECTION_DESCENDING;
            default: return TALOS_GUI_SORT_DIRECTION_NONE;
        }
    }
    return TALOS_GUI_SORT_DIRECTION_NONE;
}

TALOS_API void talos_gui_begin_group(void)
{
    BeginGroup();
}

TALOS_API void talos_gui_end_group(void)
{
    EndGroup();
}

TALOS_API void talos_gui_set_cursor_pos_x(float local_x)
{
    SetCursorPosX(local_x);
}
