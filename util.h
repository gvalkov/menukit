xcb_visualtype_t *default_visual(const xcb_screen_t *s) 
{
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
