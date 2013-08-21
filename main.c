#include <xcb/xcb.h>
#include <cairo.h>
#include <stdio.h>
#include <stdlib.h>
 
int main(void)
{
    uint32_t             mask;
    uint32_t             values[2];
    xcb_rectangle_t      r = { 20, 20, 60, 60 };
 
    // open a connection to the server
    xcb_connection_t *conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) {
        printf("Cannot open display\n");
        exit(1);
    }

    // get the first screen
    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
 
    xcb_window_t win = screen->root;

    // create black graphics context
    xcb_gcontext_t g = xcb_generate_id(conn);
    mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    values[0] = screen->black_pixel;
    values[1] = 0;
    xcb_create_gc(conn, g, win, mask, values);
 
    // create window
    win = xcb_generate_id(conn);
    mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    values[0] = screen->white_pixel;
    values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;
    xcb_create_window(conn, screen->root_depth, win, screen->root,
                      10, 10, 100, 100, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                      mask, values);
 
    // map (show) the window
    xcb_map_window(conn, win);
 
    xcb_flush(conn);
 
    // event loop
    xcb_generic_event_t *e;
    int done = 0;
    while (!done && (e = xcb_wait_for_event(conn))) {
        switch (e->response_type & ~0x80) {

            // draw or redraw the window
        case XCB_EXPOSE:
            xcb_poly_fill_rectangle(conn, win, g, 1, &r);
            xcb_flush(conn);
            break;

            // exit on key press
        case XCB_KEY_PRESS: 
            done = 1;
            break;
        }
        free(e);
    }

    // close connection to server
    xcb_disconnect(conn);
 
    return 0;
}
