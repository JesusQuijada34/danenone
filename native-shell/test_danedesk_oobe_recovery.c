/* Pruebas funcionales del cierre DaneDesk previo a la instalación del OOBE. */

#define DANEDESK_TESTING
#define main oobe_program_main
#include "first_boot_setup_gtk4.c"
#undef main

static gchar *write_fake_client(const char *response) {
    gchar *path = g_build_filename(g_get_tmp_dir(), "danedesk-oobe-test-client.sh", NULL);
    gchar *script = g_strdup_printf("#!/bin/sh\nprintf '%%s\\n' '%s'\n", response);
    g_assert_true(g_file_set_contents(path, script, -1, NULL));
    g_assert_cmpint(g_chmod(path, 0700), ==, 0);
    g_free(script);
    return path;
}

static Oobe make_oobe(void) {
    Oobe o = {0};
    o.stack = gtk_stack_new();
    o.recovery_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    o.back = gtk_button_new();
    o.next = gtk_button_new();
    o.lock_icon = gtk_image_new();
    o.stage = gtk_label_new("");
    o.status = gtk_label_new("");
    o.recovery_otp = gtk_entry_new();
    o.recovery_result = gtk_label_new("");
    g_object_ref_sink(o.stack);
    g_object_ref_sink(o.recovery_panel);
    g_object_ref_sink(o.back);
    g_object_ref_sink(o.next);
    g_object_ref_sink(o.lock_icon);
    g_object_ref_sink(o.stage);
    g_object_ref_sink(o.status);
    g_object_ref_sink(o.recovery_otp);
    g_object_ref_sink(o.recovery_result);
    gtk_widget_set_visible(o.stack, TRUE);
    gtk_widget_set_visible(o.recovery_panel, FALSE);
    return o;
}

static void free_oobe_widgets(Oobe *o) {
    g_object_unref(o->stack);
    g_object_unref(o->recovery_panel);
    g_object_unref(o->back);
    g_object_unref(o->next);
    g_object_unref(o->lock_icon);
    g_object_unref(o->stage);
    g_object_unref(o->status);
    g_object_unref(o->recovery_otp);
    g_object_unref(o->recovery_result);
}

static void set_test_environment(const char *network, const char *client_path) {
    g_setenv("DANEDESK_SERVER", "https://cloud.example", TRUE);
    g_setenv("DANEDESK_TEST_NETWORK", network, TRUE);
    if (client_path) g_setenv("DANEDESK_CLIENT_BIN", client_path, TRUE);
    else g_unsetenv("DANEDESK_CLIENT_BIN");
}

static void clear_test_environment(void) {
    g_unsetenv("DANEDESK_SERVER");
    g_unsetenv("DANEDESK_TEST_NETWORK");
    g_unsetenv("DANEDESK_CLIENT_BIN");
}

static void test_active_state_continues(void) {
    gchar *client = write_fake_client("{\"recoveryRequired\": false}");
    Oobe o = make_oobe();
    set_test_environment("online", client);

    g_assert_true(danedesk_installation_allowed(&o));
    g_assert_true(gtk_widget_get_visible(o.stack));
    g_assert_false(gtk_widget_get_visible(o.recovery_panel));

    clear_test_environment();
    g_remove(client);
    g_free(client);
    free_oobe_widgets(&o);
}

static void test_locked_or_lost_state_requires_otp(void) {
    gchar *client = write_fake_client("{\"recoveryRequired\": true}");
    Oobe o = make_oobe();
    set_test_environment("online", client);

    g_assert_false(danedesk_installation_allowed(&o));
    g_assert_false(gtk_widget_get_visible(o.stack));
    g_assert_true(gtk_widget_get_visible(o.recovery_panel));
    g_assert_nonnull(strstr(gtk_label_get_text(GTK_LABEL(o.status)), "autorización"));

    clear_test_environment();
    g_remove(client);
    g_free(client);
    free_oobe_widgets(&o);
}

static void test_missing_network_blocks_verification(void) {
    Oobe o = make_oobe();
    set_test_environment("offline", NULL);

    g_assert_false(danedesk_installation_allowed(&o));
    g_assert_true(gtk_widget_get_visible(o.stack));
    g_assert_false(gtk_widget_get_visible(o.recovery_panel));
    g_assert_nonnull(strstr(gtk_label_get_text(GTK_LABEL(o.status)), "Conecta a internet"));

    clear_test_environment();
    free_oobe_widgets(&o);
}

int main(int argc, char **argv) {
    gtk_init();
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/danedesk-oobe/active-continues", test_active_state_continues);
    g_test_add_func("/danedesk-oobe/locked-or-lost-requires-otp", test_locked_or_lost_state_requires_otp);
    g_test_add_func("/danedesk-oobe/missing-network-blocks", test_missing_network_blocks_verification);
    return g_test_run();
}
