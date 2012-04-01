#include <gtk/gtk.h>
#include <glade/glade.h>
#include "wsl-main.h"
#include "wsl-transmitter.h"
#include <libnotify/notify.h>
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include "eggtrayicon.h"
static void save_config();

void create_tray_icon(WslMain *wm);

GladeXML *xml = NULL;

EggTrayIcon *egg_tray = NULL;
GtkWidget *egg_image = NULL;
GtkListStore *store = NULL;
GKeyFile *config = NULL;

static int locked = -1;
/** 
 * Locking, unlocking the screen
 */
static void lock_screen()
{
	GError *error = NULL;
	gboolean enabled = g_key_file_get_boolean(config, "Screensaver", "enabled", &error);
	if(error)
	{
		g_key_file_set_boolean(config, "Screensaver", "enabled", TRUE);
		save_config();
		enabled = TRUE;
	}
	if(enabled)
	{
		gchar *string = g_key_file_get_string(config, "Screensaver", "lock-command", NULL);
		if(!string)
		{
			g_key_file_set_string(config, "Screensaver", "lock-command", "gnome-screensaver-command --activate");
			save_config();
			lock_screen();
			return;
		}
		g_spawn_command_line_async(string,NULL);
		g_free(string);
	}
}
static void unlock_screen()
{
	GError *error = NULL;
	gboolean enabled = g_key_file_get_boolean(config, "Screensaver", "enabled", &error);
	if(error)
	{
		g_key_file_set_boolean(config, "Screensaver", "enabled", TRUE);
		save_config();
		enabled = TRUE;
	}
	if(enabled)
	{
		gchar *string = g_key_file_get_string(config, "Screensaver", "unlock-command", NULL);
		if(!string)
		{
			g_key_file_set_string(config, "Screensaver", "unlock-command", "gnome-screensaver-command --deactivate");
			save_config();
			unlock_screen();
			return;                                                                                              	
		}
		g_spawn_command_line_async(string,NULL);
		g_free(string);
	}	
}


static void lock_script()
{
	gchar *string = g_key_file_get_string(config, "Script", "lock-command", NULL);
	if(!string)
	{
		g_key_file_set_string(config, "Script", "lock-command", "");
		save_config();
		lock_script();
		return;                                                                                              	
	}                                                                                                            	
	if(string[0] != '\0')
	{
		g_spawn_command_line_async(string,NULL);
	}
	g_free(string);
}

static void unlock_script()
{
	gchar *string = g_key_file_get_string(config, "Script", "unlock-command", NULL);
	if(!string)
	{
		g_key_file_set_string(config, "Script", "unlock-command", "");
		save_config();
		unlock_script();
		return;                                                                                              	
	}
	if(string[0] != '\0')
	{
		g_spawn_command_line_async(string,NULL);
	}
	g_free(string);
}

/**
 * Config
 */
static void save_config()
{
	gchar *path =NULL;
	gsize size;
	gchar *data = g_key_file_to_data(config, &size, NULL);
	if(data)
	{
		path = g_build_filename(g_get_home_dir(),".gwsl.conf",NULL); 
		g_file_set_contents(path, data, size, NULL);
		g_free(path);
	}
}

/**
 * Transmitter HAndling
 */
void transmitter_locked(WslTransmitter *wt, GtkTreeRowReference *row_ref)
{
	GError *error = NULL;
	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_row_reference_get_path(row_ref);
	GtkTreeModel *model = gtk_tree_row_reference_get_model(row_ref);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 1, GTK_STOCK_NO,3,"Out of Range",-1);


	gchar *name = g_strdup_printf("Transmitter with id '%i' has left this computers range", wsl_transmitter_get_id(wt));
	NotifyNotification *not = notify_notification_new("Transmitter changed",name , NULL);
	notify_notification_show(not, NULL);
	g_free(name);                    
	
	name = g_strdup_printf("%u",   wsl_transmitter_get_id(wt));
	if(g_key_file_get_boolean(config, "Dongles", name, &error) && !error)
	{
		if(locked != 1)
		{
			printf("Lock pc\n");
			lock_screen();
			lock_script();
			locked = 1;
		}
	}
	g_free(name);
}
void transmitter_unlocked(WslTransmitter *wt, GtkTreeRowReference *row_ref)
{
	GError *error = NULL;
	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_row_reference_get_path(row_ref);
	GtkTreeModel *model = gtk_tree_row_reference_get_model(row_ref);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 1, GTK_STOCK_YES,3,"In Range",-1);


	gchar *name = g_strdup_printf("Transmitter with id '%i' is in range", wsl_transmitter_get_id(wt));
	NotifyNotification *not = notify_notification_new("Transmitter changed",name , NULL);
	notify_notification_show(not, NULL);
	g_free(name);


	name = g_strdup_printf("%u",   wsl_transmitter_get_id(wt));
	if(g_key_file_get_boolean(config, "Dongles", name, &error) && !error)
	{
		if(locked == 1)
		{
			printf("UnLock pc\n");
			unlock_screen();
			unlock_script();
			locked = 0;
		 } else {
			locked = 0;
		}

	}
	g_free(name);
}

void transmitter_add(WslMain *wm, WslTransmitter *wt, GtkListStore *store)
{
	GError *error = NULL;
	GtkTreeIter iter;
	gchar *name = g_strdup_printf("%i", wsl_transmitter_get_id(wt));
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
			0, wt,
			2,name,
			4,FALSE,
			-1);
	switch(wsl_transmitter_get_in_range(wt))
	{
		case 0:
			gtk_list_store_set(store, &iter,
					1, GTK_STOCK_NO,
					3,"Out of Range",
					-1);
			break;
		case 1:
			gtk_list_store_set(store, &iter,
					1, GTK_STOCK_YES,       			
					3,"In Range",
					-1);
			break;
		default:
			gtk_list_store_set(store, &iter,
					1, GTK_STOCK_CANCEL,                                                   	
					3,"N/A",
					-1);
	}
	if(g_key_file_get_boolean(config, "Dongles", name, &error) && !error)
	{
		gtk_list_store_set(store, &iter,4,TRUE,-1);
	}	
	else if(error)
	{
		/* New dongle, not yet in config */
		g_key_file_set_boolean(config, "Dongles", name, FALSE);
		save_config();
		g_error_free(error);
	}

	GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
	GtkTreeRowReference *row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(store), path);
	g_signal_connect(G_OBJECT(wt), "locked", G_CALLBACK(transmitter_locked), row_ref);
	g_signal_connect(G_OBJECT(wt), "unlocked", G_CALLBACK(transmitter_unlocked), row_ref);

	gtk_tree_path_free(path);

	g_free(name);

}
void transmitter_added(WslMain *wm, WslTransmitter *wt, GtkListStore *store)
{
	gchar *name = NULL;
	transmitter_add(wm, wt, store);
	name = g_strdup_printf("New transmitter with id '%i' is detected\nYou can accept this transmitter in the preferences window", wsl_transmitter_get_id(wt));
	NotifyNotification *not = notify_notification_new("Transmitter found",name , NULL);
	notify_notification_show(not, NULL);
	g_free(name);
}
/**
 * Dongle handling
 */

void dongle_removed(WslMain *wm)
{
	gchar *	name = g_strdup_printf("Wireless PC Lock is now disabled");
	NotifyNotification *not = notify_notification_new("Dongle removed",name , NULL);
	notify_notification_show(not, NULL);
	g_free(name);
	gtk_image_set_from_stock(GTK_IMAGE(egg_image), GTK_STOCK_CANCEL, GTK_ICON_SIZE_MENU);
	lock_screen();
	lock_script();
}
void dongle_found(WslMain *wm, char *name)
{
	gchar *	str= g_strdup_printf("Dongle '%s' found'\nWireless PC Lock is now enabled",name);
	NotifyNotification *not = notify_notification_new("Dongle found",str , NULL);
	notify_notification_show(not, NULL);
	g_free(str);
	gtk_image_set_from_stock(GTK_IMAGE(egg_image), GTK_STOCK_YES, GTK_ICON_SIZE_MENU);
} 


void transmitter_valid_toggled(GtkCellRendererToggle *rend, gchar *path, WslMain *wm)
{
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path))
	{
		WslTransmitter *wt = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &wt, -1);
		if(wt)
		{
			gboolean state = !gtk_cell_renderer_toggle_get_active(GTK_CELL_RENDERER_TOGGLE(rend));
			gchar *name = g_strdup_printf("%i", wsl_transmitter_get_id(wt));

			g_key_file_set_boolean(config, "Dongles", name,state);
			save_config();

			gtk_list_store_set(store, &iter,4,state,-1);
			g_free(name);
		}
	}
}
void on_ch_lock_toggled(GtkToggleButton *tb)
{
	gboolean enabled = gtk_toggle_button_get_active(tb);
	g_key_file_set_boolean(config, "Screensaver", "enabled", enabled);
	save_config();
}
void build_interface(GtkWidget *item,WslMain *wm)
{
	if(xml)
	{
		gtk_widget_show_all(glade_xml_get_widget(xml, "preferences_dialog"));
	}
	else
	{
		GtkCellRenderer *renderer = NULL;
		GtkWidget *tree = NULL;
		xml = glade_xml_new(GLADE_PATH"gwsl.glade", "preferences_dialog",NULL);

		tree = glade_xml_get_widget(xml, "treeview");
		renderer = gtk_cell_renderer_toggle_new();
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), 
				-1,"Valid", renderer,
				"active", 4,NULL);
		g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(transmitter_valid_toggled), wm);
		renderer = gtk_cell_renderer_pixbuf_new();
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), 
				-1,"", renderer,
				"stock-id", 1,NULL);

		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), 
				-1,"Id", renderer,                                   	
				"text", 2,NULL);
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), 
				-1,"State", renderer,                                   	
				"text", 3,NULL);


		gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));


		{
			GError *error = NULL;
			gboolean enabled = g_key_file_get_boolean(config, "Screensaver", "enabled", &error);
			if(error)
			{
				g_key_file_set_boolean(config, "Screensaver", "enabled", TRUE);
				save_config();
				enabled = TRUE;
			}
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "ch_lock")),enabled);
		}





		gtk_widget_show_all(glade_xml_get_widget(xml, "preferences_dialog"));
		glade_xml_signal_autoconnect(xml);
	}
}


static void tray_popup_menu(GtkWidget *widget, GdkEventButton *event, WslMain *wm)
{
	int button = event->button;
	int activate_time = event->time;
	if(button == 3)
	{
		GtkWidget *item = NULL;
		GtkWidget *menu = gtk_menu_new();

		/** Lock */
		item = gtk_image_menu_item_new_with_label("Lock");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_MENU));
		//g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(tray_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		/** separator */
		item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		/** quit */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(build_interface), wm);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);


		/** separator */
		item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		/** quit */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(gtk_main_quit), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		/* popup */
		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, activate_time);
	}
}
static void tray_destroyed(GtkWidget *tray, WslMain *wm)
{
	printf("Tray destroyed\n");
	egg_tray = NULL;
	create_tray_icon(wm);	
	if(wsl_main_has_dongle(wm))
	{
		gtk_image_set_from_stock(GTK_IMAGE(egg_image), GTK_STOCK_YES, GTK_ICON_SIZE_MENU);
	}
}

void create_tray_icon(WslMain *wm)
{
	GtkWidget*event = gtk_event_box_new();
	egg_image = gtk_image_new_from_stock(GTK_STOCK_CANCEL, GTK_ICON_SIZE_MENU);
	egg_tray = egg_tray_icon_new("Wireless PC Lock");
	gtk_container_add(GTK_CONTAINER(egg_tray), event);
	gtk_container_add(GTK_CONTAINER(event), egg_image);
	gtk_widget_show_all(GTK_WIDGET(egg_tray));
	g_signal_connect(G_OBJECT(event), "button-press-event", G_CALLBACK(tray_popup_menu), wm);
	g_signal_connect(G_OBJECT(egg_tray), "destroy", G_CALLBACK(tray_destroyed), wm);
}

int main(int argc, char **argv)
{
	gchar *path;
	WslMain *wm = NULL;
	WslTransmitter *wt = NULL;
	gtk_init(&argc, &argv);
	notify_init("Wireless PC Lock");
	config = g_key_file_new();
	path = g_build_filename(g_get_home_dir(),".gwsl.conf",NULL); 
	g_key_file_load_from_file(config, path, 0, NULL);
	g_free(path);

	/**
	 * Make a default store
	 */
	store = gtk_list_store_new(5, G_TYPE_POINTER,/*the pointer to the widget */
			G_TYPE_STRING,/* icon string */
			G_TYPE_STRING,/* name */
			G_TYPE_STRING,
			G_TYPE_BOOLEAN);/** allowed to lock */
	/** 
	 * Start 
	 */
	wm = wsl_main_new();
	/** just testing */
	/** add known dongles */
	{
		gsize size;
		int i;
		char ** keys = g_key_file_get_keys(config, "Dongles", &size, NULL);
		for(i=0;keys && i<size;i++)
		{
			wt = wsl_main_add_transmitter(wm, atoi(keys[i]));
			transmitter_add(wm, wt,store);
		}
	}

	g_signal_connect(G_OBJECT(wm), "transmitter_found", 
			G_CALLBACK(transmitter_added), 
			store);
	g_signal_connect(G_OBJECT(wm), "device_found", 
			G_CALLBACK(dongle_found), NULL);
	g_signal_connect(G_OBJECT(wm), "device_removed",
			G_CALLBACK(dongle_removed), NULL);

	create_tray_icon(wm);
	gtk_main();
	save_config();
	return 0;
}
