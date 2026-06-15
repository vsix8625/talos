#pragma once
#include <stddef.h>

typedef enum
{
    TALOS_GUI_FLAG_NONE = 0,

    TALOS_GUI_WINDOW_NO_TITLE_BAR               = 1 << 0,
    TALOS_GUI_WINDOW_NO_RESIZE                  = 1 << 1,
    TALOS_GUI_WINDOW_NO_MOVE                    = 1 << 2,
    TALOS_GUI_WINDOW_NO_SCROLLBAR               = 1 << 3,
    TALOS_GUI_WINDOW_NO_SCROLL_WITH_MOUSE       = 1 << 4,
    TALOS_GUI_WINDOW_NO_COLLAPSE                = 1 << 5,
    TALOS_GUI_WINDOW_ALWAYS_AUTO_RESIZE         = 1 << 6,
    TALOS_GUI_WINDOW_NO_BACKGROUND              = 1 << 7,
    TALOS_GUI_WINDOW_NO_SAVED_SETTINGS          = 1 << 8,
    TALOS_GUI_WINDOW_NO_MOUSE_INPUTS            = 1 << 9,
    TALOS_GUI_WINDOW_MENU_BAR                   = 1 << 10,
    TALOS_GUI_WINDOW_NO_FOCUS_ON_APPEARING      = 1 << 12,
    TALOS_GUI_WINDOW_NO_BRING_TO_FRONT_ON_FOCUS = 1 << 13,
    TALOS_GUI_WINDOW_NO_NAV_INPUTS              = 1 << 16,
    TALOS_GUI_WINDOW_NO_NAV_FOCUS               = 1 << 17,

    TALOS_GUI_WINDOW_NO_NAV = TALOS_GUI_WINDOW_NO_NAV_INPUTS | TALOS_GUI_WINDOW_NO_NAV_FOCUS,

    TALOS_GUI_WINDOW_NO_DECORATION = TALOS_GUI_WINDOW_NO_TITLE_BAR | TALOS_GUI_WINDOW_NO_RESIZE |
                                     TALOS_GUI_WINDOW_NO_SCROLLBAR | TALOS_GUI_WINDOW_NO_COLLAPSE,

    TALOS_GUI_WINDOW_NO_INPUTS = TALOS_GUI_WINDOW_NO_MOUSE_INPUTS | TALOS_GUI_WINDOW_NO_NAV_INPUTS |
                                 TALOS_GUI_WINDOW_NO_NAV_FOCUS
} talos_gui_flags;

typedef enum
{
    TALOS_GUI_SORT_DIRECTION_NONE       = 0,
    TALOS_GUI_SORT_DIRECTION_ASCENDING  = 1,
    TALOS_GUI_SORT_DIRECTION_DESCENDING = 2,
} talos_gui_sort_direction;

typedef enum
{
    TALOS_TABLE_FLAG_NONE                 = 0,
    TALOS_TABLE_FLAG_RESIZABLE            = 1 << 0,
    TALOS_TABLE_FLAG_REORDERABLE          = 1 << 1,
    TALOS_TABLE_FLAG_HIDEABLE             = 1 << 2,
    TALOS_TABLE_FLAG_SORTABLE             = 1 << 3,
    TALOS_TABLE_FLAG_NO_SAVED_SETTINGS    = 1 << 4,
    TALOS_TABLE_FLAG_CONTEXT_MENU_IN_BODY = 1 << 5,
    TALOS_TABLE_FLAG_ROW_BG               = 1 << 6,
    TALOS_TABLE_FLAG_BORDERS_INNER_H      = 1 << 7,
    TALOS_TABLE_FLAG_BORDERS_OUTER_H      = 1 << 8,
    TALOS_TABLE_FLAG_BORDERS_INNER_V      = 1 << 9,
    TALOS_TABLE_FLAG_BORDERS_OUTER_V      = 1 << 10,

    TALOS_TABLE_FLAG_BORDERS_H =
        TALOS_TABLE_FLAG_BORDERS_INNER_H | TALOS_TABLE_FLAG_BORDERS_OUTER_H,

    TALOS_TABLE_FLAG_BORDERS_V =
        TALOS_TABLE_FLAG_BORDERS_INNER_V | TALOS_TABLE_FLAG_BORDERS_OUTER_V,

    TALOS_TABLE_FLAG_BORDERS_INNER =
        TALOS_TABLE_FLAG_BORDERS_INNER_V | TALOS_TABLE_FLAG_BORDERS_INNER_H,

    TALOS_TABLE_FLAG_BORDERS_OUTER =
        TALOS_TABLE_FLAG_BORDERS_OUTER_V | TALOS_TABLE_FLAG_BORDERS_OUTER_H,

    TALOS_TABLE_FLAG_BORDERS = TALOS_TABLE_FLAG_BORDERS_INNER | TALOS_TABLE_FLAG_BORDERS_OUTER,

    TALOS_TABLE_NO_BORDERS_IN_BODY             = 1 << 11,
    TALOS_TABLE_NO_BORDERS_IN_BODY_TILL_RESIZE = 1 << 12,
    TALOS_TABLE_FLAG_SIZING_FIXED_FIT          = 1 << 13,
    TALOS_TABLE_FLAG_SIZING_FIXED_SAME         = 2 << 13,
    TALOS_TABLE_FLAG_SIZING_STRETCH_PROP       = 3 << 13,
    TALOS_TABLE_FLAG_SIZING_STRETCH_SAME       = 4 << 13,
    TALOS_TABLE_FLAG_NO_HOST_EXTEND_X          = 1 << 16,
    TALOS_TABLE_FLAG_NO_HOST_EXTEND_Y          = 1 << 17,
    TALOS_TABLE_FLAG_NO_KEEP_COLUMNS_VIS       = 1 << 18,
    TALOS_TABLE_FLAG_PRECISE_WIDTHS            = 1 << 19,
    TALOS_TABLE_FLAG_NO_CLIP                   = 1 << 20,
    TALOS_TABLE_FLAG_PAD_OUTER_X               = 1 << 21,
    TALOS_TABLE_FLAG_NO_PAD_OUTER_X            = 1 << 22,
    TALOS_TABLE_FLAG_NO_PAD_INNER_X            = 1 << 23,
    TALOS_TABLE_FLAG_SCROLL_X                  = 1 << 24,
    TALOS_TABLE_FLAG_SCROLL_Y                  = 1 << 25,
    TALOS_TABLE_FLAG_SORT_MULTI                = 1 << 26,
    TALOS_TABLE_FLAG_SORT_TRISTATE             = 1 << 27,
    TALOS_TABLE_FLAG_HIGHLIGHT_HOVERED_COL     = 1 << 28,
} talos_table_flags;

typedef enum
{
    TALOS_TABLE_COLUMN_FLAG_NONE          = 0,
    TALOS_TABLE_COLUMN_FLAG_DEFAULT_SORT  = 1 << 2,
    TALOS_TABLE_COLUMN_FLAG_WIDTH_STRETCH = 1 << 3,
    TALOS_TABLE_COLUMN_FLAG_WIDTH_FIXED   = 1 << 4,
    TALOS_TABLE_COLUMN_FLAG_NO_SORT       = 1 << 9,
} talos_table_column_flags;

typedef enum talos_imgui_key
{
    TALOS_KEY_NONE = 0,

    TALOS_KEY_TAB         = 512,  // ImGuiKey_NamedKey_BEGIN
    TALOS_KEY_LEFT_ARROW  = 513,
    TALOS_KEY_RIGHT_ARROW = 514,
    TALOS_KEY_UP_ARROW    = 515,
    TALOS_KEY_DOWN_ARROW  = 516,
    TALOS_KEY_ENTER       = 525,
    TALOS_KEY_ESCAPE      = 526,
    TALOS_KEY_0           = 536,
    TALOS_KEY_1,
    TALOS_KEY_2,
    TALOS_KEY_3,
    TALOS_KEY_4,
    TALOS_KEY_5,
    TALOS_KEY_6,
    TALOS_KEY_7,
    TALOS_KEY_8,
    TALOS_KEY_9,
    TALOS_KEY_A     = 546,
    TALOS_KEY_C     = 548,
    TALOS_KEY_D     = 549,
    TALOS_KEY_F11   = 578,
    TALOS_KEY_F12   = 579,
    TALOS_MOD_CTRL  = 1 << 12,
    TALOS_MOD_SHIFT = 1 << 13,
    TALOS_MOD_ALT   = 1 << 14
} talos_imgui_key;

#define TALOS_GUI_STYLE_FRAME_PADDING 3

#ifdef __cplusplus
    #define TALOS_API extern "C"
TALOS_API
{
#else
    #define TALOS_API
#endif

    int  talos_gui_init(void *window, void *gl_ctx);
    void talos_gui_shutdown(void);

    void talos_gui_new_frame(void);
    void talos_gui_render(void);

    void talos_gui_begin_frame(void);
    void talos_gui_render_frame(void);

    int  talos_gui_begin(const char *title, int *open, int flags);
    void talos_gui_end(void);

    void talos_gui_text(const char *text);
    int  talos_gui_button(const char *label);
    int  talos_gui_checkbox(const char *label, int *v);
    int  talos_gui_slider_float(const char *label, float *v, float min, float max);
    int  talos_gui_input_text(const char *label, char *buf, size_t buf_size);

    void talos_gui_separator(void);
    void talos_gui_same_line(void);
    void talos_gui_spacing(void);

    void  talos_gui_set_next_window_pos(float x, float y);
    void  talos_gui_set_next_window_size(float x, float y);
    void  talos_gui_set_next_window_bg_alpha(float alpha);
    void  talos_gui_progress_bar(float fraction, const char *overlay);
    float talos_gui_get_framerate(void);

    int  talos_gui_begin_child(const char *str_id, float size_x, float size_y, int border);
    void talos_gui_end_child(void);

    void talos_gui_push_font_large(void);
    void talos_gui_push_font_small(void);
    void talos_gui_pop_font(void);

    void talos_gui_push_style_var_float2(int idx, float x, float y);
    void talos_gui_pop_style_var(int count);

    int  talos_gui_begin_table(const char *id, int columns, int flags, float outer_size_y);
    void talos_gui_end_table(void);
    void talos_gui_table_next_row(void);
    int  talos_gui_table_set_column_index(int col);
    void talos_gui_table_setup_column(const char *label, int flags, float width);
    void talos_gui_table_setup_scroll_freeze(int cols, int rows);
    void talos_gui_table_headers_row(void);

    bool talos_gui_table_get_sort_specs_dirty(void);
    void talos_gui_table_clear_sort_specs_dirty(void);
    int  talos_gui_table_get_sort_column(void);

    int talos_gui_event_process(void *event);

    float talos_gui_get_scroll_y(void);
    void  talos_gui_set_scroll_y(float y);

    void talos_gui_push_id_int(int int_id);
    void talos_gui_pop_id(void);

    float talos_gui_get_scroll_y(void);
    void  talos_gui_set_scroll_y(float y);
    void  talos_gui_push_id_int(int int_id);
    void  talos_gui_pop_id(void);

    bool talos_gui_selectable(const char *label, bool selected, int flags);
    bool talos_gui_is_item_clicked(int mouse_button);

    void talos_gui_open_popup(const char *str_id, int popup_flags);
    bool talos_gui_begin_popup_modal(const char *name, bool *p_open, int flags);
    void talos_gui_end_popup(void);
    void talos_gui_close_current_popup(void);

    int talos_gui_table_get_sort_column(void);
    int talos_gui_table_get_sort_direction(void);

    void talos_gui_begin_group(void);
    void talos_gui_end_group(void);

    void talos_gui_set_cursor_pos_x(float local_x);

    bool talos_gui_is_key_pressed(int talos_key);

#ifdef __cplusplus
}
#endif
