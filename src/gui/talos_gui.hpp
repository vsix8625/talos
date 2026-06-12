#pragma once
#include <stddef.h>

typedef enum
{
    TALOS_GUI_FLAG_NONE = 0,

    TALOS_GUI_WINDOW_NO_TITLE_BAR          = 1 << 0,
    TALOS_GUI_WINDOW_NO_RESIZE             = 1 << 1,
    TALOS_GUI_WINDOW_NO_MOVE               = 1 << 2,
    TALOS_GUI_WINDOW_NO_SCROLLBAR          = 1 << 3,
    TALOS_GUI_WINDOW_NO_SCROLL_WITH_MOUSE  = 1 << 4,
    TALOS_GUI_WINDOW_NO_COLLAPSE           = 1 << 5,
    TALOS_GUI_WINDOW_ALWAYS_AUTO_RESIZE    = 1 << 6,
    TALOS_GUI_WINDOW_NO_BACKGROUND         = 1 << 7,
    TALOS_GUI_WINDOW_NO_SAVED_SETTINGS     = 1 << 8,
    TALOS_GUI_WINDOW_NO_MOUSE_INPUTS       = 1 << 9,
    TALOS_GUI_WINDOW_MENU_BAR              = 1 << 10,
    TALOS_GUI_WINDOW_NO_FOCUS_ON_APPEARING = 1 << 12,
    TALOS_GUI_WINDOW_NO_NAV_INPUTS         = 1 << 16,
    TALOS_GUI_WINDOW_NO_NAV_FOCUS          = 1 << 17,

    TALOS_GUI_WINDOW_NO_NAV = TALOS_GUI_WINDOW_NO_NAV_INPUTS | TALOS_GUI_WINDOW_NO_NAV_FOCUS,

    TALOS_GUI_WINDOW_NO_DECORATION = TALOS_GUI_WINDOW_NO_TITLE_BAR | TALOS_GUI_WINDOW_NO_RESIZE |
                                     TALOS_GUI_WINDOW_NO_SCROLLBAR | TALOS_GUI_WINDOW_NO_COLLAPSE,

    TALOS_GUI_WINDOW_NO_INPUTS = TALOS_GUI_WINDOW_NO_MOUSE_INPUTS | TALOS_GUI_WINDOW_NO_NAV_INPUTS |
                                 TALOS_GUI_WINDOW_NO_NAV_FOCUS
} talos_gui_flags;

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

#ifdef __cplusplus
}
#endif
