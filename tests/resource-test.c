#include <dbus/dbus.h>
#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <res-conn.h>

#include "resource.h"

void dummy_callback(resource_set_t *resource_set,
                    uint32_t        resources,
                    void           *userdata)
{
}

START_TEST (test_resource_set_create_and_destroy)
{
	resource_set_t *rs;

	// 1.1. should fail with invalid args
	fail_if(( rs = resource_set_create("player", RESOURCE_AUDIO_PLAYBACK, 0, 0, NULL          , 0)) != NULL );
	// 1.2. should succeed with valid args
	fail_if(( rs = resource_set_create("player", RESOURCE_AUDIO_PLAYBACK, 0, 0, dummy_callback, 0)) == NULL );

	resource_set_destroy(rs);
}
END_TEST

START_TEST (test_resource_set_configure_resources)
{
	resource_set_t *rs;

	rs = resource_set_create("player", RESOURCE_AUDIO_PLAYBACK, 0, 0, dummy_callback, 0);
	resource_set_configure_resources(rs, FALSE, TRUE);
	resource_set_destroy(rs);
}
END_TEST

START_TEST (test_resource_set_configure_advice_callback)
{
	resource_set_t *rs;

	rs = resource_set_create("player", RESOURCE_AUDIO_PLAYBACK, 0, 0, dummy_callback, 0);
	resource_set_configure_advice_callback(rs, dummy_callback, NULL);
	resource_set_destroy(rs);
}
END_TEST

START_TEST (test_resource_set_acquire_and_release)
{
	resource_set_t *rs;

	rs = resource_set_create("player", RESOURCE_AUDIO_PLAYBACK, 0, 0, dummy_callback, 0);
	resource_set_acquire(rs);
	resource_set_release(rs);
	resource_set_destroy(rs);
}
END_TEST

START_TEST (test_resource_set_configure_audio)
{
	resource_set_t *rs;

	rs = resource_set_create("player", RESOURCE_AUDIO_PLAYBACK, 0, 0, dummy_callback, 0);
	resource_set_configure_audio(rs, "player", 0, "stream1");
	resource_set_destroy(rs);
}
END_TEST



TCase *
libresource_case (int desired_step_id)
{
	int step_id = 1;
	#define PREPARE_TEST(tc, fun) if (desired_step_id == 0 || step_id++ == desired_step_id)  tcase_add_test (tc, fun);

	TCase *tc_libresource = tcase_create ("libresource");
    tcase_set_timeout(tc_libresource, 60);
    PREPARE_TEST (tc_libresource, test_resource_set_create_and_destroy);
    PREPARE_TEST (tc_libresource, test_resource_set_configure_resources);
    PREPARE_TEST (tc_libresource, test_resource_set_configure_advice_callback);
    PREPARE_TEST (tc_libresource, test_resource_set_acquire_and_release);
    PREPARE_TEST (tc_libresource, test_resource_set_configure_audio);

    return tc_libresource;
}

int
main(int argc, char* argv[]) {
	Suite *s = suite_create ("libresource");

	int step_id = 0;
	if (argc == 2) {
		step_id = strtol(argv[1], NULL, 10);
	}
	suite_add_tcase (s, libresource_case(step_id));

	SRunner *sr = srunner_create (s);
	srunner_run_all (sr, CK_VERBOSE);
	int number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return number_failed;
}


/* mocks */

////////////////////////////////////////////////////////////////
resconn_t *resourceConnection;
resset_t  *resSet;

static void verify_resproto_init(resproto_role_t role,
                                 resproto_transport_t transport,
                                 resconn_linkup_t callbackFunction,
                                 DBusConnection *dbusConnection)
{
    DBusConnection *systemBus;
    systemBus = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);

    fail_unless(callbackFunction != NULL);
    fail_unless(dbusConnection == systemBus);
    fail_unless(role == RESPROTO_ROLE_CLIENT);
    fail_unless(transport == RESPROTO_TRANSPORT_DBUS);
}

resconn_t* resproto_init(resproto_role_t role, resproto_transport_t transport, ...)
{
    resconn_linkup_t callbackFunction;
    DBusConnection *dbusConnection;
    va_list args;

    va_start(args, transport);
    callbackFunction = va_arg(args, resconn_linkup_t);
    dbusConnection = va_arg(args, DBusConnection *);
    va_end(args);

    verify_resproto_init(role, transport, callbackFunction, dbusConnection);

    resourceConnection =(resconn_t *) calloc(1, sizeof(resconn_t));

    return resourceConnection;
}

static void verify_resconn_connect(resconn_t *connection, resmsg_t *message,
                                   resproto_status_t callbackFunction)
{
    fail_unless(connection == resourceConnection);
    fail_unless(message->record.type == RESMSG_REGISTER);
    fail_unless(message->record.id == 1);
    fail_unless(message->record.reqno == 1);
    fail_unless(message->record.rset.all == (RESMSG_AUDIO_PLAYBACK|RESMSG_AUDIO_RECORDING
                                            |RESMSG_VIDEO_PLAYBACK|RESMSG_VIDEO_RECORDING));
    fail_unless(message->record.rset.opt == (RESMSG_AUDIO_RECORDING|RESMSG_VIDEO_PLAYBACK
                                            |RESMSG_VIDEO_RECORDING));
    fail_unless(message->record.rset.share == 0);
    fail_unless(message->record.rset.mask == 0);
    fail_unless(strcmp(message->record.klass, "player") == 0);
    fail_unless(message->record.mode == 0);
    fail_unless(callbackFunction != NULL);
}

resset_t  *resconn_connect(resconn_t *connection, resmsg_t *message,
                           resproto_status_t callbackFunction)
{
    verify_resconn_connect(connection, message, callbackFunction);

    resSet = (resset_t *) calloc(1, sizeof(resset_t));

    return resSet;
}

char *resmsg_res_str(uint32_t res, char *buf, int len)
{
    snprintf(buf, len, "0x%04x", res);

    return buf;
}

int resproto_set_handler(union resconn_u *r, resmsg_type_t t, resproto_handler_t h)
{
    return 1;
}

char *resmsg_type_str(resmsg_type_t type)
{
    char *str;

    switch (type) {
    case RESMSG_REGISTER:      str = "register";         break;
    case RESMSG_UNREGISTER:    str = "unregister";       break;
    case RESMSG_UPDATE:        str = "update";           break;
    case RESMSG_ACQUIRE:       str = "acquire";          break;
    case RESMSG_RELEASE:       str = "releaase";         break;
    case RESMSG_GRANT:         str = "grant";            break;
    case RESMSG_ADVICE:        str = "advice";           break;
    case RESMSG_AUDIO:         str = "audio";            break;
    case RESMSG_STATUS:        str = "status";           break;
    default:                   str = "<unknown type>";   break;
    }

    return str;
}

int resproto_send_message(resset_t          *rset,
                          resmsg_t          *resmsg,
                          resproto_status_t  status)
{
    resconn_t       *rcon = rset->resconn;
    resmsg_type_t    type = resmsg->type;
    int              success;

    if (rset->state != RESPROTO_RSET_STATE_CONNECTED ||
        type == RESMSG_REGISTER || type == RESMSG_UNREGISTER)
        success = FALSE;
    else {
        resmsg->any.id = rset->id;
        success = rcon->any.send(rset, resmsg, status);
    }

    return success;
}


DBusConnection *resource_get_dbus_bus(DBusBusType type, DBusError *err)
{
    DBusConnection *conn = NULL;

    if ((conn = dbus_bus_get(type, err)) != NULL) {
        dbus_connection_setup_with_g_main(conn, NULL);
    }

    return conn;
}