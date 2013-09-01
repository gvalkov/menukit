#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

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

#include "lua_util.h"
#include "lua_dsl.h"
#include "lua_defaults.h"
#include "lua_post.h"


typedef struct menu {
    xcb_window_t win;
    struct menu *parent;
    const char* name;
} menu;

typedef struct menuitem {
    menu *parent;

} menuitem;


// Globals
const char* version = "0.1";
xcb_connection_t *conn = NULL;
menu *menus = NULL;



// Utility functions
static void debug(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}

static int die_lua(lua_State *L) {
    const char *msg = luaL_checkstring(L, 1);
    int exitcode = luaL_checknumber(L, 2);
    fwrite(msg, sizeof(char), strlen(msg), stderr);
    fwrite("\n", 1, 1, stderr);
    exit(exitcode);
    return 0;
}


// Event handlers
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
    xcb_motion_notify_event_t *mouse = NULL;

    while ((event = xcb_poll_for_event(conn))) {
        if (XCB_EVENT_RESPONSE_TYPE(event) == XCB_MOTION_NOTIFY) {
            mouse = (xcb_motion_notify_event_t*)event;
            debug("%d %d\n", mouse->event_x, mouse->event_y);
        }
    }
}

static void event_handler(struct ev_loop *loop, ev_io *w, int revents) {
}


// Drawing and window handling
xcb_visualtype_t *default_visual(const xcb_screen_t *s) {
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(s);

    if(depth_iter.data)
        for(; depth_iter.rem; xcb_depth_next (&depth_iter))
            for(xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
                visual_iter.rem; xcb_visualtype_next (&visual_iter))
                if(s->root_visual == visual_iter.data->visual_id)
                    return visual_iter.data;

    return NULL;
}

xcb_screen_t* screen_get(xcb_connection_t* conn, int screen) {
    xcb_screen_t *s;
    s = xcb_aux_get_screen(conn, screen);
    return s;
}

static xcb_window_t create_window(xcb_connection_t *conn, xcb_screen_t *screen, const uint32_t color) {
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
        XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_POINTER_MOTION |
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

static char* parse_options(int argc, char **argv) {
    static const char usage[] =
        "Usage: menus [-hdv] [-c config.lua]\n"
        "\n"
        "Menus is a lightweight menu system for X11.\n"
        "\n"
        "Required arguments:\n"
        "  -c, --config path        lua config file\n"
        "\n"
        "Optional arguments:\n"
        "  -h, --help               show this help message and exit\n"
        "  -d, --debug              show debug messages\n"
        "  -v, --version            show program's version number and exit\n"
        ;

    int debug_flag = 0;
    int help_flag = 0;
    int version_flag = 0;
    char *config_file = NULL;

    if (argc == 1) {
        fwrite(usage, 1, sizeof(usage), stdout);
        exit(1);
    }

    while (1) {
        int c, idx=0;

        struct option long_options[] = {
            {"debug",   no_argument, &debug_flag, 1},
            {"help",    no_argument, &help_flag, 1},
            {"version", no_argument, &version_flag, 1},
            {"config",  required_argument, 0, 'c'},
            {0,0,0,0},
        };

        c = getopt_long(argc, argv, "dhvc:", long_options, &idx);
        if (c == -1) break;
     
        switch (c) {
        case 0: if (long_options[idx].flag != 0) break;
        case 'h': help_flag = 1; break;
        case 'd': debug_flag = 1; break;
        case 'v': version_flag =1; break;
        case 'c': config_file = optarg; break;
        case '?': break;
        default: die("invalid");
        }
    }

    if (help_flag) {
        fwrite(usage, 1, sizeof(usage), stdout);
        exit(1);
    }

    if (version_flag) {
        printf("menus version %s\n", version);
        exit(0);
    }

    if (config_file == NULL)
        die("error: missing required argument '-c, --config'\n");

    if (access(config_file, F_OK | R_OK) == -1)
        die("error: config file '%s' does not exist or is not readable.\n", config_file);

    return config_file;
}

static void load_config(const char* config_file, lua_State* L) {
    lua_pushcfunction(L, die_lua);
    lua_setglobal(L, "die");

    
    if (luaL_loadbuffer(L, (const char*)lua_util, sizeof(lua_util), "src/util.lua") || lua_pcall(L, 0, 0, 0))
        die("error: cannot load helper functions '%s'", lua_tostring(L, -1));

    if (luaL_loadbuffer(L, (const char*)lua_dsl, sizeof(lua_dsl), "src/dsl.lua") || lua_pcall(L, 0, 0, 0))
        die("error: cannot load helper functions '%s'", lua_tostring(L, -1));

    if (luaL_loadfile(L, config_file) || lua_pcall(L, 0, 0, 0))
        die("error: cannot run configuration file '%s'", lua_tostring(L, -1));

    if (luaL_loadbuffer(L, (const char*)lua_post, sizeof(lua_post), "src/post.lua") || lua_pcall(L, 0, 0, 0))
        die("error: cannot load helper functions '%s'", lua_tostring(L, -1));

    lua_getglobal(L, "menus");
    if (!lua_istable(L, -1)) 
        die("global variable 'menus' not defined or is not a table\n");

    lua_pushnil(L);
    debug("%d\n", lua_objlen(L, -2));
    while (lua_next(L, -2) != 0) {
        printf("%s - %s\n",
               lua_typename(L, lua_type(L, -2)),
               lua_typename(L, lua_type(L, -1)));
        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(L, 1);
    }
}


int main(int argc, char **argv) {
    char *config_file = parse_options(argc, argv);
    debug("debug: using config file '%s'\n", config_file);
    
    // initialize lua 
    lua_State *L = lua_open();
    luaL_openlibs(L);

    load_config(config_file, L);
    
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
