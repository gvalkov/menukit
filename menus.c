#include <ev.h>
#include <cairo.h>
#include <cairo-xcb.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_ewmh.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "util.h"

xcb_connection_t *conn = NULL;

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}

struct program_options {
    char *title;
    char *class;
};

const char *usage =
    "usage: menus asdfasdfas dfasdfa sdfasdf"
    "\n"
    "asdfasdf";

static void exit_handler(struct ev_loop *loop, ev_signal *w, int revents) {
    ev_break(loop, EVBREAK_ALL);
    printf("received something\n");
    exit(1);
}

static void reload_handler(struct ev_loop *loop, ev_signal *w, int revents) {
    ev_break(loop, EVBREAK_ALL);
    printf("received something\n");
    exit(1);
}

static void event_prehandler(struct ev_loop *loop, ev_check *w, int revents) {
    xcb_generic_event_t *event;
    xcb_generic_event_t *mouse = NULL;

    while((event = xcb_poll_for_event(conn)))
    {
        if(XCB_EVENT_RESPONSE_TYPE(event) == XCB_MOTION_NOTIFY)
            mouse = event;
        // else
        //     xcb_event_handle(&rootconf.event_h, event);
    }
    if(mouse)
    {
        puts("mouse");
        // xcb_event_handle(&rootconf.event_h, mouse);
    }
}

static void event_handler(struct ev_loop *loop, ev_io *w, int revents) {
    
    printf("received an event\n");
    exit(1);
}

// another callback, this time for a time-out
// static void timeout_cb (struct ev_loop *loop, ev_timer *w, int revents) {
//     puts("timeout");
//     ev_break(loop, EVBREAK_ONE);
// }

static xcb_window_t create_window(xcb_connection_t *conn,
                                  xcb_screen_t *screen, const uint32_t color) {
    uint32_t mask = 0;
    uint32_t values[3];

    xcb_window_t win = xcb_generate_id(conn);

    mask |= XCB_CW_BACK_PIXEL;
    values[0] = color;

    mask |= XCB_CW_OVERRIDE_REDIRECT;
    values[1] = 1;

    mask |= XCB_CW_EVENT_MASK;
    values[2] = XCB_EVENT_MASK_EXPOSURE |
        XCB_EVENT_MASK_KEY_PRESS |
        XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_VISIBILITY_CHANGE |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(
        conn, XCB_COPY_FROM_PARENT, win, screen->root,
        600, 600,
        // screen->width_in_pixels, screen->height_in_pixels,
        300, 300,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        mask, values);

    xcb_map_window(conn, win);
    // set_window_ontop(conn, win);

    return win;
}

static void setup_window(xcb_connection_t *conn, xcb_window_t win, xcb_ewmh_connection_t *ewmh, char *title) {
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(title), title);
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(title), title);
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win, ewmh->_NET_WM_WINDOW_TYPE, XCB_ATOM_ATOM, 32, 1, &(ewmh->_NET_WM_WINDOW_TYPE_POPUP_MENU));

    uint32_t values[1];
    values[0] = XCB_STACK_MODE_ABOVE;
    xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_STACK_MODE, values);

}

static void draw(xcb_connection_t *conn) {
}

static struct ev_loop* setup_events(xcb_connection_t *conn) {
    struct ev_loop *loop = EV_DEFAULT;

    struct ev_signal sigint, sighup, sigterm;
    ev_signal_init(&sigint,  exit_handler, SIGINT);
    ev_signal_init(&sigterm, exit_handler, SIGTERM);
    ev_signal_init(&sighup,  reload_handler, SIGHUP);

    ev_signal_start(loop, &sigint);
    ev_signal_start(loop, &sighup);
    ev_signal_start(loop, &sigterm);

    // ev_unref(loop);
    // ev_unref(loop);
    // ev_unref(loop);

    ev_io xcb_io;
    ev_check xcb_check;

    int xcb_fd = xcb_get_file_descriptor(conn);
    ev_io_init(&xcb_io, event_handler, xcb_fd, EV_READ);
    ev_io_start(loop, &xcb_io);
    ev_check_init(&xcb_check, &event_prehandler);
    ev_check_start(loop, &xcb_check);

    return loop;
}


int main(void)
{
    // configure lua 
    lua_State *L = lua_open();
    luaL_openlibs(L);
    lua_close(L);

    // configure events

    // open a connection to the server
    conn = xcb_connect(NULL, NULL);
    if (!conn || xcb_connection_has_error(conn))
        die("Cannot open display\n");

    // intern ewmh atoms
    xcb_ewmh_connection_t *ewmh = malloc(sizeof(xcb_ewmh_connection_t));
    xcb_intern_atom_cookie_t *ewmh_cookie = xcb_ewmh_init_atoms(conn, ewmh);
    xcb_ewmh_init_atoms_replies(ewmh, ewmh_cookie, NULL);

    const xcb_setup_t *setup = xcb_get_setup(conn);
    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    xcb_window_t win = create_window(conn, screen, (uint32_t)(0x101010));

    // set window properties
    setup_window(conn, win, ewmh, "Menus");
    xcb_flush(conn);

    // draw
    xcb_visualtype_t *visual = default_visual(screen);
    cairo_surface_t *surface = cairo_xcb_surface_create(conn, win, visual, 300, 300);
    cairo_t *ctx = cairo_create(surface);

    cairo_set_source_rgb(ctx, 1, 0, 1);
    cairo_rectangle(ctx, 0, 0, 300, 300);
    cairo_fill(ctx);
    cairo_surface_destroy(surface);
    cairo_destroy(ctx);
    xcb_flush(conn);

    
    struct ev_loop *loop = setup_events(conn);
    ev_run(loop, 0);

    // close connection to server
    xcb_disconnect(conn);
    return 0;
}
