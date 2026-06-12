#include "talos_gui.hpp"
#include "glad.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

using namespace ImGui;

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
