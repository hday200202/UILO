/* Keep the private libdecor core in the executable's dependencies so SDL's
 * runtime loader reuses it. The normal system decoration plugin is unchanged. */
__attribute__((visibility("default"))) void uilo_link_gnome_decor(void) {}
