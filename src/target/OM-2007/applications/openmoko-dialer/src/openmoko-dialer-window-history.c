/*   openmoko-dialer-window-talking.c
 *
 *  Authored by Tony Guan<tonyguan@fic-sh.com.cn>
 *
 *  Copyright (C) 2006 FIC Shanghai Lab
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Public License as published by
 *  the Free Software Foundation; version 2.1 of the license.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  Current Version: $Rev$ ($Date) [$Author: Tony Guan $]
 */

#include <libmokoui/moko-finger-tool-box.h>
#include <libmokoui/moko-finger-window.h>
#include <libmokoui/moko-finger-wheel.h>
#include <libmokoui/moko-pixmap-button.h>

#include <gtk/gtkalignment.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreemodelfilter.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtkmenu.h>
#include "contacts.h"
#include "openmoko-dialer-main.h"
#include "moko-dialer-status.h"
#include  "history.h"
#include "openmoko-dialer-window-history.h"

/**
 * @brief re-filter the treeview widget by the history type
 *
 * 
 *
 * @param type HISTORY_TYPE, indicating only the history items of that type will be displayed
 * @return 1
 * @retval
 */
 
int history_view_change_filter(MOKO_DIALER_APP_DATA* p_dialer_data,HISTORY_TYPE  type)
{
GtkWidget* historyview;
GtkTreePath* path;
DBG_TRACE();
p_dialer_data->g_history_filter_type=type;
gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(p_dialer_data->g_list_store_filter));

path=gtk_tree_path_new_first();
gtk_tree_view_set_cursor(GTK_TREE_VIEW(p_dialer_data->treeview_history),path,0,0);   
gtk_tree_path_free(path);

	
return 1;
}

void
on_all_calls_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	MOKO_DIALER_APP_DATA* p_dialer_data=(MOKO_DIALER_APP_DATA* )user_data;
	GtkWidget* label=p_dialer_data->label_filter_history;
	gtk_label_set_text(GTK_LABEL(label),"All");
	history_view_change_filter(p_dialer_data,ALL);
	history_update_counter(p_dialer_data);
}


void
on_missed_calls_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	MOKO_DIALER_APP_DATA* p_dialer_data=(MOKO_DIALER_APP_DATA* )user_data;
	GtkWidget* label=p_dialer_data->label_filter_history;
	gtk_label_set_text(GTK_LABEL(label),"Missed");
	history_view_change_filter(p_dialer_data,MISSED);
	history_update_counter(p_dialer_data);
}


void
on_dialed_calls_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	MOKO_DIALER_APP_DATA* p_dialer_data=(MOKO_DIALER_APP_DATA* )user_data;
	GtkWidget* label=p_dialer_data->label_filter_history;
	gtk_label_set_text(GTK_LABEL(label),"Dialed");
	history_view_change_filter(p_dialer_data,OUTGOING);
	history_update_counter(p_dialer_data);
}


void
on_received_calls_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	MOKO_DIALER_APP_DATA* p_dialer_data=(MOKO_DIALER_APP_DATA* )user_data;
	GtkWidget* label=p_dialer_data->label_filter_history;
	gtk_label_set_text(GTK_LABEL(label),"Received");
	history_view_change_filter(p_dialer_data,INCOMING);
	history_update_counter(p_dialer_data);
}

gboolean
on_eventboxTop_button_release_event    (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        MOKO_DIALER_APP_DATA * appdata)
{
	
 gtk_menu_popup(GTK_MENU(appdata->menu_history),0,0,0,0,0,0);
 
  return FALSE;
}

void
openmoko_history_wheel_press_left_up_cb(GtkWidget *widget, MOKO_DIALER_APP_DATA * appdata)
{
DBG_ENTER();
 GtkTreeSelection    *selection;
 GtkTreeModel        *model;
 GtkTreeIter         iter;
 GtkTreePath* path;
 GtkTreeView * treeview;
 //DBG_ENTER();

treeview=GTK_TREE_VIEW(appdata->treeview_history);
 if(treeview==0)return ;
	 
 selection = gtk_tree_view_get_selection(treeview);

 if (!gtk_tree_selection_get_selected(selection, &model, &iter))
 {
	 DBG_WARN("no current selection\n");
	 return ;
 }
 		path=gtk_tree_model_get_path(model,&iter);
		if(!gtk_tree_path_prev(path))
		{
				DBG_WARN("no prev for the top level\n");
				gtk_tree_path_free(path);
				return;
		}
		gtk_tree_view_set_cursor(treeview,path,0,0);
  return ;


}

void
openmoko_history_wheel_press_right_down_cb(GtkWidget *widget, MOKO_DIALER_APP_DATA * appdata)
{
DBG_ENTER();
 GtkTreeSelection    *selection;
 GtkTreeModel        *model;
 GtkTreeIter         iter;
 GtkTreePath* path;
 GtkTreeView * treeview;
 //DBG_ENTER();
treeview=GTK_TREE_VIEW(appdata->treeview_history);
 if(treeview==0)return ;
	 
 selection = gtk_tree_view_get_selection(treeview);

 if (!gtk_tree_selection_get_selected(selection, &model, &iter))
 {
	 DBG_WARN("no current selection\n");
	 return ;
 }
	 if(gtk_tree_model_iter_next(model,&iter))
	 {
		path=gtk_tree_model_get_path(model,&iter);
		gtk_tree_view_set_cursor(treeview,path,0,0);
		gtk_tree_path_free(path);
		return ;
	 }
 
  return ;
}


void cb_tool_button_history_delete_clicked( GtkButton* button, MOKO_DIALER_APP_DATA * appdata)
{
GtkTreeIter iter; //iter of the filter store
GtkTreeIter iter0; //iter of the back store
GtkTreeModel* model;
GtkTreeModel* model0;
GtkTreeSelection * selection;
GtkTreeView* treeview;
GtkTreePath* path;
treeview=GTK_TREE_VIEW(appdata->treeview_history);
selection = gtk_tree_view_get_selection(treeview);

	
if (!gtk_tree_selection_get_selected(selection, &model, &iter))
{
	return;
}
	
if(appdata->g_currentselected)
{
DBG_MESSAGE("to delete %s",appdata->g_currentselected->number);
history_delete_entry(&(appdata->g_historylist),appdata->g_currentselected);
}

path=gtk_tree_model_get_path(model,&iter);


model0=gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model));


gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(model),&iter0,&iter);

gtk_list_store_remove(GTK_LIST_STORE(model0),&iter0);


gtk_tree_view_set_cursor(treeview,path,0,0);


if (!gtk_tree_selection_get_selected(selection, &model, &iter))
{
	if(!gtk_tree_path_prev(path))
	{
		gtk_tree_view_set_cursor(treeview,path,0,0);
		DBG_WARN("history is empty now!");
		history_update_counter(appdata);
	}
	else
	{
		gtk_tree_view_set_cursor(treeview,path,0,0);
	}
	//we deleted the last one.
	
}

gtk_tree_path_free(path);

return ;

  DBG_ENTER();
}

void cb_tool_button_history_call_clicked( GtkButton* button, MOKO_DIALER_APP_DATA * appdata)
{
	DBG_ENTER();
	

  
}

void cb_tool_button_history_sms_clicked( GtkButton* button, MOKO_DIALER_APP_DATA * appdata)
{
	DBG_ENTER();
	

  
}

void cb_tool_button_history_back_clicked( GtkButton* button, MOKO_DIALER_APP_DATA * appdata)
{
     gtk_widget_hide(appdata->window_history);
     
}


void
on_window_history_hide                 (GtkWidget       *widget,
                                        MOKO_DIALER_APP_DATA * appdata)
{

gtk_widget_hide(appdata->wheel_history);
gtk_widget_hide(appdata->toolbox_history);

}

void
on_window_history_show                  (GtkWidget       *widget,
                                        MOKO_DIALER_APP_DATA * appdata)
{
DBG_ENTER();

if(appdata->wheel_history)
gtk_widget_show(appdata->wheel_history);

if(appdata->toolbox_history)
gtk_widget_show(appdata->toolbox_history);

if(appdata->history_need_to_update)
{
DBG_MESSAGE("NEED TO UPDATE HISTORY");
}
   
DBG_LEAVE();
}




gint window_history_init( MOKO_DIALER_APP_DATA* p_dialer_data)
{

DBG_ENTER();


if(p_dialer_data->window_history==0)
{

	 history_create_menu_history (p_dialer_data);

	MokoFingerWindow* window=NULL;
      MokoFingerToolBox *tools = NULL;
	GtkWidget* button;
	GtkWidget* image;

//now the container--window
     window = MOKO_FINGER_WINDOW(moko_finger_window_new());
     p_dialer_data->window_history=window;

    
     moko_finger_window_set_contents(window, create_window_history_content(p_dialer_data));

     g_signal_connect ((gpointer) window, "show",
	                    G_CALLBACK (on_window_history_show),
	                    p_dialer_data);
    g_signal_connect ((gpointer) window, "hide",
	                    G_CALLBACK (on_window_history_hide),
	                    p_dialer_data);

    
     
     gtk_widget_show_all( GTK_WIDGET(window) );

//the gtk_widget_show_all is really bad, cause i have to call it and then hide some widgets.


 //now the wheel and tool box, why should the wheel and toolbox created after the gtk_widget_show_all???
   gtk_widget_show(GTK_WIDGET(moko_finger_window_get_wheel(window)));
    
    g_signal_connect(G_OBJECT(moko_finger_window_get_wheel(window)),
		    "press_left_up",
		    G_CALLBACK(openmoko_history_wheel_press_left_up_cb),
		    p_dialer_data);
    g_signal_connect(G_OBJECT(moko_finger_window_get_wheel(window)),
		    "press_right_down",
		    G_CALLBACK(openmoko_history_wheel_press_right_down_cb),
		    p_dialer_data);



        tools = moko_finger_window_get_toolbox(window);
     
	button = moko_finger_tool_box_add_button_without_label(tools);
       image = file_new_image_from_relative_path("phone.png");
	moko_pixmap_button_set_finger_toolbox_btn_center_image(button, image);
        g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(cb_tool_button_history_call_clicked), p_dialer_data);
	
	button = moko_finger_tool_box_add_button_without_label(tools);
       image = file_new_image_from_relative_path("sms.png");
	moko_pixmap_button_set_finger_toolbox_btn_center_image(button, image);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(cb_tool_button_history_sms_clicked), p_dialer_data);
	

	button = moko_finger_tool_box_add_button_without_label(tools);
       image = file_new_image_from_relative_path("delete.png");
	moko_pixmap_button_set_finger_toolbox_btn_center_image(button, image);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(cb_tool_button_history_delete_clicked), p_dialer_data);
	gtk_widget_show(GTK_WIDGET(tools));

	button = moko_finger_tool_box_add_button_without_label(tools);
       image = file_new_image_from_relative_path("tony.png");
	moko_pixmap_button_set_finger_toolbox_btn_center_image(button, image);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(cb_tool_button_history_back_clicked), p_dialer_data);
	gtk_widget_show(GTK_WIDGET(tools));
	
	p_dialer_data->wheel_history=moko_finger_window_get_wheel(window);
	p_dialer_data->toolbox_history=tools;

	gtk_widget_hide(window);

	DBG_LEAVE();
}
else
{
	//here we have to refresh it.
	DBG_TRACE();
}
    return 1;
}

void on_treeviewHistory_cursor_changed      (GtkTreeView     *treeview,
                                        gpointer         user_data)
{
GtkTreeIter iter;
GtkTreeModel* model;
GtkTreeSelection * selection;
HISTORY_ENTRY * p;
int hasname;
MOKO_DIALER_APP_DATA* p_dialer_data=(MOKO_DIALER_APP_DATA* )user_data;
	
selection = gtk_tree_view_get_selection(p_dialer_data->treeview_history);
 
if (!gtk_tree_selection_get_selected(selection, &model, &iter))
 {
 	 p_dialer_data->g_currentselected=0;
	 return ;
 }
	
gtk_tree_model_get(model,&iter,COLUMN_ENTRYPOINTER,&p,-1);

 p_dialer_data->g_currentselected=p;

gtk_tree_model_get(model,&iter,COLUMN_HASNAME,&hasname,-1);
history_update_counter(p_dialer_data);

}



GtkWidget* create_window_history_content (MOKO_DIALER_APP_DATA* p_dialer_data)
{

  GtkWidget *treeviewHistory;
  GtkWidget *vbox=gtk_vbox_new(FALSE,0); 
  //FIRST of all, the top title area;
  GtkWidget *eventboxTop = gtk_event_box_new ();
  gtk_widget_show (eventboxTop);
  gtk_box_pack_start(GTK_BOX(vbox),eventboxTop,FALSE,FALSE,5);
  gtk_widget_set_size_request (eventboxTop, 480, 64);
  gtk_widget_set_name(eventboxTop,"gtkeventbox-black");
	
  GtkWidget *  hbox67 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox67);
  gtk_container_add (GTK_CONTAINER (eventboxTop), hbox67);

  GtkWidget * eventboxLeftTop = gtk_event_box_new ();
  gtk_widget_show (eventboxLeftTop);
  gtk_box_pack_start (GTK_BOX (hbox67), eventboxLeftTop, FALSE, TRUE, 0);
  gtk_widget_set_name(eventboxLeftTop,"gtkeventbox-black");



  GtkWidget * imageLeftMenu =file_new_image_from_relative_path("all.png");
  gtk_widget_show (imageLeftMenu);
  gtk_container_add (GTK_CONTAINER (eventboxLeftTop), imageLeftMenu);

  GtkWidget *labelHistoryTitle = gtk_label_new (("History-"));
  gtk_widget_show (labelHistoryTitle);
  gtk_box_pack_start (GTK_BOX (hbox67), labelHistoryTitle, FALSE, FALSE, 0);
  gtk_widget_set_size_request (labelHistoryTitle, 221, -1);
  gtk_label_set_justify (GTK_LABEL (labelHistoryTitle), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (labelHistoryTitle), 1, 0.5);

  GtkWidget *labelFilter = gtk_label_new (("All"));
  gtk_widget_show (labelFilter);
  gtk_box_pack_start (GTK_BOX (hbox67), labelFilter, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (labelFilter), 0, 0.5);
  p_dialer_data->label_filter_history=labelFilter;

  GtkWidget *labelCounter = gtk_label_new (("1/21"));
  gtk_widget_show (labelCounter);
  gtk_box_pack_start (GTK_BOX (hbox67), labelCounter, TRUE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (labelCounter), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (labelCounter), 0.8, 0.5);
  p_dialer_data->label_counter_history=labelCounter;
 
    g_signal_connect ((gpointer) eventboxTop, "button_release_event",
                    G_CALLBACK (on_eventboxTop_button_release_event),
                    p_dialer_data);


  GtkWidget *scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
 
  treeviewHistory = gtk_tree_view_new ();
  gtk_container_add (GTK_CONTAINER (scrolledwindow), treeviewHistory);

  gtk_widget_show (treeviewHistory);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeviewHistory), FALSE);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (treeviewHistory), FALSE);
//   gtk_misc_set_alignment (GTK_MISC (treeviewHistory), 0.5, 0.5);
  gtk_box_pack_start(GTK_BOX(vbox),scrolledwindow,TRUE,TRUE,0);

  gtk_widget_set_name(treeviewHistory,"gtktreeview-black");
  p_dialer_data->treeview_history=treeviewHistory;
   history_build_history_list_view(p_dialer_data);
  gtk_widget_set_size_request (scrolledwindow, -1, 400);    
//  gtk_misc_set_alignment (GTK_MISC (treeviewHistory),1,0.1);


  g_signal_connect ((gpointer) treeviewHistory, "cursor_changed",
                    G_CALLBACK (on_treeviewHistory_cursor_changed),
                    p_dialer_data);


  return vbox;
}


/**
 * @brief re-filter the treeview widget by current history type,a callback when the history treeview refreshes
 *
 * this callback will be called for every treemodel iters,whenever the treeview filter is refreshing
 *
 * @param model GtkTreeModel *, the background database of the treeview
 * @param iter GtkTreeIter *, the iterator of every item of the model.
 * @param  data gpointer , of no use currently
 * @return boolean
 * @retval TRUE means the iter will be displayed
 * @retval  FALSE means the iter will not be displayed
 */
static gboolean history_view_filter_visible_function(GtkTreeModel *model,GtkTreeIter *iter,gpointer data)
{
	MOKO_DIALER_APP_DATA* p_dialer_data=(MOKO_DIALER_APP_DATA* )data;
	HISTORY_TYPE type;
	if(p_dialer_data->g_history_filter_type==ALL)
		return TRUE;
	gtk_tree_model_get(model,iter,COLUMN_TYPE,&type,-1);
	if(type==p_dialer_data->g_history_filter_type)
		return TRUE;
	else
		return FALSE;
}



/**
 * @brief find the treeview in the window, fill-in the data and show it on the screen.
 *
 *
 *
 * @param window GtkWidget* the window which contains the history treeview. but it's not necessarilly
 *to be a window, any widget that can help to lookup the treeview will be OK.
 * @return 
 * @retval 0 error occured
 * @retval 1 everything is OK
 */

gint history_build_history_list_view(MOKO_DIALER_APP_DATA* p_dialer_data)
{
	GtkListStore * list_store;
	
	GtkTreeIter iter;
	HISTORY_ENTRY * entry;
	
	//copied
	GtkTreeViewColumn   *col;
  	GtkCellRenderer     *renderer;
  	
  	//GtkTreeModel        *model;
	GtkWidget           *contactview=NULL;
	
	//DBG_ENTER();
	//DBG_MESSAGE("History:%d",g_historylist.length);
	
	//DBG_TRACE();
  p_dialer_data->g_history_filter_type=ALL;
  contactview = p_dialer_data->treeview_history;

  if(contactview == NULL)
    return 0;
//pack image and label
  col = gtk_tree_view_column_new ();

  gtk_tree_view_column_set_title (col, ("Title"));
  gtk_tree_view_column_set_resizable (col, TRUE);
 
	
  
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_tree_view_column_set_attributes (col, renderer, 
  "pixbuf",COLUMN_TYPEICON,NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_tree_view_column_set_attributes(col, renderer, 
  "text",COLUMN_TIME,NULL);
 
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_tree_view_column_set_attributes(col, renderer, 
  "text",COLUMN_SEPRATE,NULL);
 
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_attributes(col, renderer, 
  "text",COLUMN_NAME_NUMBER,NULL);


   gtk_tree_view_append_column (GTK_TREE_VIEW (contactview), col);


	entry=p_dialer_data->g_historylist.first;
	
	list_store=gtk_list_store_new(N_COLUMN,	G_TYPE_INT,GDK_TYPE_PIXBUF,
					G_TYPE_STRING,	G_TYPE_STRING,	
					G_TYPE_STRING,	G_TYPE_INT,G_TYPE_POINTER,G_TYPE_INT,-1);
	//we will use a filter to facilitate the filtering in treeview without rebuilding the database.				
	p_dialer_data->g_list_store_filter=gtk_tree_model_filter_new(GTK_TREE_MODEL(list_store),NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(p_dialer_data->g_list_store_filter),
	history_view_filter_visible_function,p_dialer_data,NULL);

//load the three icons to memory.
GError *error=NULL;	
		p_dialer_data->g_iconReceived=create_pixbuf("received.png",&error);
  		if(error)
				{
					DBG_WARN("Cound not load icon :%s",error->message);
					g_error_free(error);
					p_dialer_data->g_iconReceived=NULL;
					error=NULL;
				}

		p_dialer_data->g_iconDialed=create_pixbuf("dialed.png",&error);
		if(error)
				{
					DBG_WARN("Cound not load icon :%s",error->message);
					g_error_free(error);
					p_dialer_data->g_iconDialed=NULL;
					error=NULL;
				}

  
		 p_dialer_data->g_iconMissed=create_pixbuf("missed.png",&error);
		if(error)
				{
					DBG_WARN("Cound not load icon :%s",error->message);
					g_error_free(error);
					p_dialer_data->g_iconMissed=NULL;
					error=NULL;
				}


	while(entry)
	{
		//DBG_MESSAGE(entry->number);
		gtk_list_store_append(list_store,&iter);
		gtk_list_store_set(list_store,&iter,COLUMN_TYPE,entry->type,COLUMN_SEPRATE,"--",
		COLUMN_TIME,entry->time,COLUMN_DURATION,entry->durationsec,COLUMN_ENTRYPOINTER,entry,COLUMN_HASNAME,0,-1);
		if(entry->name==0)
		{
		//DBG_MESSAGE(entry->number);
		gtk_list_store_set(list_store,&iter,COLUMN_NAME_NUMBER,entry->number,-1);
		gtk_list_store_set(list_store,&iter,COLUMN_HASNAME,0,-1);
		}
		else
		{
		gtk_list_store_set(list_store,&iter,COLUMN_NAME_NUMBER,entry->name,-1);		
		gtk_list_store_set(list_store,&iter,COLUMN_HASNAME,1,-1);
		}
		switch(entry->type)
		{
			case INCOMING:
			{
				gtk_list_store_set(list_store,&iter,COLUMN_TYPEICON,p_dialer_data->g_iconReceived,-1);		
			//	icon=gdk_pixbuf_new_from_file("./received.png",&error);
				break;
			}
			case OUTGOING:
			{//	icon=gdk_pixbuf_new_from_file("./dialed.png",&error);
				gtk_list_store_set(list_store,&iter,COLUMN_TYPEICON,p_dialer_data->g_iconDialed,-1);		
				break;
			}
			case MISSED:
			{	//icon=gdk_pixbuf_new_from_file("./missed.png",&error);
				gtk_list_store_set(list_store,&iter,COLUMN_TYPEICON,p_dialer_data->g_iconMissed,-1);		
				break;
			}

			default:
			
			{	//icon=gdk_pixbuf_new_from_file("./missed.png",&error);
				gtk_list_store_set(list_store,&iter,COLUMN_TYPEICON,p_dialer_data->g_iconMissed,-1);		
				break;
			}
		}
						

		
		entry=entry->next;
	}
	
  gtk_tree_view_set_model (GTK_TREE_VIEW(contactview),GTK_TREE_MODEL(p_dialer_data->g_list_store_filter));

  g_object_unref (list_store);

  return 1;
}


/**
 * @brief update the counter display widget - labelCounter
 *
 * @param widget GtkWidget*, any widget in the same window with treeviewHistory and labelCounter
 *
 * @return 1
 */
gint history_update_counter(MOKO_DIALER_APP_DATA* p_dialer_data)
{
DBG_ENTER();
GtkTreeIter iter;
GtkTreeModel* model;
GtkTreeSelection * selection;
GtkTreePath *path;
GtkTreeView * treeview;
int count=0;
int nth=0;
char * pathstring;
char display[10];
	
treeview=GTK_TREE_VIEW(p_dialer_data->treeview_history);
if(!p_dialer_data->treeview_history)
{
DBG_WARN("COUNTER NOT READY ");
return 0;
}


model=gtk_tree_view_get_model(treeview);	

count=gtk_tree_model_iter_n_children(model,NULL);
	
selection = gtk_tree_view_get_selection(treeview);
 
if (!gtk_tree_selection_get_selected(selection, &model, &iter))
{
	nth=0;
}
else
{
path=gtk_tree_model_get_path(model,&iter);
pathstring=gtk_tree_path_to_string(path);
nth=atoi(pathstring)+1;
gtk_tree_path_free(path);	

}
	
GtkWidget *labelcounter;
labelcounter=p_dialer_data->label_counter_history;
sprintf(display,"%d/%d",nth,count);
gtk_label_set_text(GTK_LABEL(labelcounter),display);
return 1;	

}

GtkWidget* history_create_menu_history (MOKO_DIALER_APP_DATA* p_dialer_data)
{
if(!p_dialer_data->menu_history)
{
  GtkWidget *menu_history;
  GtkWidget *all_calls;
  GtkWidget *imageAll;
  GtkWidget *separator1;
  GtkWidget *missed_calls;
  GtkWidget *imageMissed;
  GtkWidget *separator3;
  GtkWidget *dialed_calls;
  GtkWidget *imageDialed;
  GtkWidget *separator2;
  GtkWidget *received_calls;
  GtkWidget *imageReceived;

  menu_history = gtk_menu_new ();
  gtk_container_set_border_width (GTK_CONTAINER (menu_history), 2);

  all_calls = gtk_image_menu_item_new_with_mnemonic (("Calls All"));
  gtk_widget_show (all_calls);
  gtk_container_add (GTK_CONTAINER (menu_history), all_calls);
  gtk_widget_set_size_request (all_calls, 250, 60);


  imageAll =file_new_image_from_relative_path("all.png");
  gtk_widget_show (imageAll);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (all_calls), imageAll);

  separator1 = gtk_separator_menu_item_new ();
  gtk_widget_show (separator1);
  gtk_container_add (GTK_CONTAINER (menu_history), separator1);
  gtk_widget_set_size_request (separator1, 120, -1);
  gtk_widget_set_sensitive (separator1, FALSE);

  missed_calls = gtk_image_menu_item_new_with_mnemonic (("Calls Missed "));
  gtk_widget_show (missed_calls);
  gtk_container_add (GTK_CONTAINER (menu_history), missed_calls);
  gtk_widget_set_size_request (missed_calls, 120, 60);

  //imageMissed = gtk_image_new_from_stock ("gtk-goto-bottom", GTK_ICON_SIZE_MENU);
  imageMissed =file_new_image_from_relative_path("missed.png");
  
  gtk_widget_show (imageMissed);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (missed_calls), imageMissed);

  separator3 = gtk_separator_menu_item_new ();
  gtk_widget_show (separator3);
  gtk_container_add (GTK_CONTAINER (menu_history), separator3);
  gtk_widget_set_size_request (separator3, 120, -1);
  gtk_widget_set_sensitive (separator3, FALSE);

  dialed_calls = gtk_image_menu_item_new_with_mnemonic (("Calls Dialed"));
  gtk_widget_show (dialed_calls);
  gtk_container_add (GTK_CONTAINER (menu_history), dialed_calls);
  gtk_widget_set_size_request (dialed_calls, 120, 60);

 // imageDialed = gtk_image_new_from_stock ("gtk-go-up", GTK_ICON_SIZE_MENU);
  imageDialed =file_new_image_from_relative_path("dialed.png");
  gtk_widget_show (imageDialed);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (dialed_calls), imageDialed);

  separator2 = gtk_separator_menu_item_new ();
  gtk_widget_show (separator2);
  gtk_container_add (GTK_CONTAINER (menu_history), separator2);
  gtk_widget_set_size_request (separator2, 120, -1);
  gtk_widget_set_sensitive (separator2, FALSE);

  received_calls = gtk_image_menu_item_new_with_mnemonic (("Calls Received "));
  gtk_widget_show (received_calls);
  gtk_container_add (GTK_CONTAINER (menu_history), received_calls);
  gtk_widget_set_size_request (received_calls, 120, 60);

//  imageReceived = gtk_image_new_from_stock ("gtk-go-down", GTK_ICON_SIZE_MENU);
 imageReceived=file_new_image_from_relative_path("received.png");
  gtk_widget_show (imageReceived);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (received_calls), imageReceived);

  g_signal_connect ((gpointer) all_calls, "activate",
                    G_CALLBACK (on_all_calls_activate),
                    p_dialer_data);
  g_signal_connect ((gpointer) missed_calls, "activate",
                    G_CALLBACK (on_missed_calls_activate),
                    p_dialer_data);
  g_signal_connect ((gpointer) dialed_calls, "activate",
                    G_CALLBACK (on_dialed_calls_activate),
                    p_dialer_data);
  g_signal_connect ((gpointer) received_calls, "activate",
                    G_CALLBACK (on_received_calls_activate),
                    p_dialer_data);

p_dialer_data->menu_history=menu_history;
  return menu_history;
}
else
	return p_dialer_data->menu_history;
}



/**
 * @brief add an entry to the history treeview
 *
 *
 *
 * @param entry HISTORY_ENTRY *, the history entry to be added to the treeview.
 * @return 
 * @retval 0 error occured
 * @retval 1 everything is OK
 */
gint history_list_view_add(MOKO_DIALER_APP_DATA* appdata,HISTORY_ENTRY * entry)
{
DBG_ENTER();
if(entry==0)
	{
		DBG_ERROR("THE ENTRY IS ZERO");
		return 0;
	}

if(appdata->treeview_history==0)
{
		DBG_WARN("not ready");
		return 0;

}
	//
GtkTreeIter iter; //iter of the filter store
GtkTreeModel* model;
GtkListStore* list_store;
GtkTreeView* treeview;

treeview=GTK_TREE_VIEW(appdata->treeview_history);

model=gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(appdata->g_list_store_filter));	

list_store=GTK_LIST_STORE(model);
	//
	
	
	//DBG_MESSAGE(entry->number);
		gtk_list_store_insert(list_store,&iter,0);
		gtk_list_store_set(list_store,&iter,COLUMN_TYPE,entry->type,COLUMN_SEPRATE,"--",
		COLUMN_TIME,entry->time,COLUMN_DURATION,entry->durationsec,COLUMN_ENTRYPOINTER,entry,COLUMN_HASNAME,0,-1);
		if(entry->name==0)
		{
		//DBG_MESSAGE(entry->number);
		gtk_list_store_set(list_store,&iter,COLUMN_NAME_NUMBER,entry->number,-1);
		gtk_list_store_set(list_store,&iter,COLUMN_HASNAME,0,-1);
		}
		else
		{
		gtk_list_store_set(list_store,&iter,COLUMN_NAME_NUMBER,entry->name,-1);		
		gtk_list_store_set(list_store,&iter,COLUMN_HASNAME,1,-1);
		}
		switch(entry->type)
		{
			case INCOMING:
			{
				gtk_list_store_set(list_store,&iter,COLUMN_TYPEICON,appdata->g_iconReceived,-1);		
			//	icon=gdk_pixbuf_new_from_file("./received.png",&error);
				break;
			}
			case OUTGOING:
			{//	icon=gdk_pixbuf_new_from_file("./dialed.png",&error);
				gtk_list_store_set(list_store,&iter,COLUMN_TYPEICON,appdata->g_iconDialed,-1);		
				break;
			}
			case MISSED:
			{	//icon=gdk_pixbuf_new_from_file("./missed.png",&error);
				gtk_list_store_set(list_store,&iter,COLUMN_TYPEICON,appdata->g_iconMissed,-1);		
				break;
			}

			default:
			
			{	//icon=gdk_pixbuf_new_from_file("./missed.png",&error);
				gtk_list_store_set(list_store,&iter,COLUMN_TYPEICON,appdata->g_iconMissed,-1);		
				break;
			}
		}
history_update_counter(appdata); 
return 1;	
}


gint add_histroy_entry(MOKO_DIALER_APP_DATA* appdata,HISTORY_TYPE type,const char *name,const char *number,const char *picpath,  char *time,char *date,int durationsec)
{
//DBG_ENTER();

	//DBG_MESSAGE("History add:%s,%s,%s,%s,%s,%d",name,number,picpath,time,date,durationsec);
 HISTORY_ENTRY * pentry=history_add_entry(&(appdata->g_historylist),type,name,number,picpath,time,date,durationsec);	
 return history_list_view_add(appdata,pentry);	
}


