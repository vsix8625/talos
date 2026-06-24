#include "talos_gui.h"
#include "glad.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include "imgui_internal.h"

#include <SDL3/SDL.h>

extern "C"
{
#include "../../external/vx/include/vx.h"
#include <fontconfig/fontconfig.h>
}

using namespace ImGui;

static ImFont *g_imgui_font_small = nullptr;
static ImFont *g_imgui_font_large = nullptr;

const char *get_system_monospace_font(void)
{
    FcConfig  *config = FcInitLoadConfigAndFonts();
    FcPattern *pat    = FcNameParse((const FcChar8 *) "monospace");
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    FcResult   result;
    FcPattern *match = FcFontMatch(config, pat, &result);

    static char font_path[VX_PATH_MAX];
    font_path[0] = '\0';

    if (match)
    {
        FcChar8 *file = NULL;
        if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch)
        {
            snprintf(font_path, sizeof(font_path), "%s", (char *) file);
        }
        FcPatternDestroy(match);
    }

    FcPatternDestroy(pat);
    FcConfigDestroy(config);
    FcFini();

    return font_path[0] != '\0' ? font_path : NULL;
}

TALOS_API int talos_gui_init(void *window, void *gl_ctx, int width)
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

    char exe_path[VX_BUF_SIZE_4096];
    char distributed_font_path[VX_PATH_MAX] = {0};

    if (vx_platform_get_self_exe(exe_path, sizeof(exe_path)) == VX_OK)
    {
        char *last_slash = strrchr(exe_path, '/');
        if (last_slash)
        {
            *last_slash = '\0';  // Chop off the executable name

            snprintf(distributed_font_path,
                     sizeof(distributed_font_path),
                     "%s/JetBrainsMonoNerdFont-Medium.ttf",
                     exe_path);
        }
    }

    vx_dbglog("SELF_PATH: %s", exe_path);

    const char *system_monospace = get_system_monospace_font();

    const char *font_search_paths[] = {
        distributed_font_path, "assets/fonts/JetBrainsMonoNerdFont-Medium.ttf", system_monospace};

    const char *resolved_path = nullptr;
    u32         path_count    = sizeof(font_search_paths) / sizeof(font_search_paths[0]);

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

    if (resolved_path)
    {
        if (width >= 1920)
        {
            g_imgui_font_small = io.Fonts->AddFontFromFileTTF(resolved_path, 28.0f);
            g_imgui_font_large = io.Fonts->AddFontFromFileTTF(resolved_path, 36.0f);
        }
        else
        {
            g_imgui_font_small = io.Fonts->AddFontFromFileTTF(resolved_path, 20.0f);
            g_imgui_font_large = io.Fonts->AddFontFromFileTTF(resolved_path, 28.0f);
        }

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

TALOS_API int talos_gui_button_xy(const char *label, float x, float y)
{
    return Button(label, (ImVec2) {x, y});
}

TALOS_API int talos_gui_small_button(const char *label)
{
    return SmallButton(label);
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

TALOS_API void talos_gui_same_line_offset(float offset_from_start_x)
{
    SameLine(offset_from_start_x);
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

TALOS_API void talos_gui_push_style_var(i32 idx, f32 var)
{
    PushStyleVar(idx, var);
}

TALOS_API void talos_gui_push_style_var_float2(i32 idx, f32 x, f32 y)
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

TALOS_API bool talos_gui_is_key_pressed(int talos_key)
{
    return IsKeyPressed((ImGuiKey) talos_key, false);
}

TALOS_API void talos_gui_push_style_color(talos_gui_color_idx idx, vx_vec4f color)
{
    PushStyleColor((ImGuiCol) idx, ImVec4(color.r, color.g, color.b, color.a));
}

TALOS_API void talos_gui_pop_style_color(int count)
{
    PopStyleColor(count);
}

TALOS_API void talos_gui_text_disabled(const char *text)
{
    TextDisabled("%s", text);
}

TALOS_API void talos_gui_text_link(const char *label, const char *url)
{
    ImVec4 link_color       = ImVec4(0.30f, 0.60f, 0.90f, 1.00f);  // Bright blue
    ImVec4 link_hover_color = ImVec4(0.50f, 0.80f, 1.00f, 1.00f);  // Hover cyan flare

    ImGuiID       id = GetID(label);
    ImGuiContext &g  = *GImGui;

    bool is_hovered = (g.HoveredId == id);

    PushStyleColor(ImGuiCol_Text, is_hovered ? link_hover_color : link_color);
    TextUnformatted(label);
    PopStyleColor(1);

    if (IsItemHovered())
    {
        SetMouseCursor(ImGuiMouseCursor_Hand);

        if (IsMouseClicked(ImGuiMouseButton_Left))
        {
            SDL_OpenURL(url);
        }
    }
}

TALOS_API void talos_gui_plot_lines(const char *label,
                                    float (*getter)(void *data, int idx),
                                    void       *data,
                                    int         values_count,
                                    const char *overlay_text,
                                    float       scale_min,
                                    float       scale_max,
                                    float       graph_width,
                                    float       graph_height)
{
    PlotLines(label,
              getter,
              data,
              values_count,
              0,
              overlay_text,
              scale_min,
              scale_max,
              (ImVec2) {graph_width, graph_height});
}

TALOS_API void talos_gui_list_clipper_begin(talos_gui_list_clipper *clipper, i32 count)
{
    ImGuiListClipper *c = (ImGuiListClipper *) clipper;
    c->Begin(count);
}

TALOS_API bool talos_gui_list_clipper_step(talos_gui_list_clipper *clipper)
{
    return ((ImGuiListClipper *) clipper)->Step();
}

TALOS_API void talos_gui_list_clipper_end(talos_gui_list_clipper *clipper)
{
    ((ImGuiListClipper *) clipper)->End();
}

TALOS_API i32 talos_gui_list_clipper_display_start(talos_gui_list_clipper *clipper)
{
    return ((ImGuiListClipper *) clipper)->DisplayStart;
}

TALOS_API i32 talos_gui_list_clipper_display_end(talos_gui_list_clipper *clipper)
{
    return ((ImGuiListClipper *) clipper)->DisplayEnd;
}

TALOS_API void talos_gui_set_keyboard_focus_here(void)
{
    SetKeyboardFocusHere(0);
}

TALOS_API bool talos_gui_want_capture_keyboard(void)
{
    return GetIO().WantCaptureKeyboard;
}
