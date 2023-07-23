#include <gtk/gtk.h>

#include <vector>
#include <string>

#include "creator.cpp"
#include "recorder.cpp"
#include "shared.h"

GMutex shared_mutex;
GThread* recorder_thread = nullptr;
std::vector<std::string> windows;
HWND selectedWindow = nullptr;

void start_recorder_thread() {
  recorder_thread = g_thread_new("recorder_thread", start_recorder, selectedWindow);
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    char title[80];
    GetWindowText(hwnd,title,sizeof(title));
    // get hwnd id

    // Check if window is visible and has a non-zero length title
    if(IsWindowVisible(hwnd) && strlen(title) != 0)
    {
      std::vector<std::string>* windows = reinterpret_cast<std::vector<std::string>*>(lParam);
      windows->push_back(std::string(title));
    }
    
    return TRUE;
}

void window_selected(GtkComboBox *widget, gpointer user_data) {
  // get selected window
  int index = gtk_combo_box_get_active(widget);
  g_mutex_lock(&shared_mutex);
  if(index == 0) {
    selectedWindow = nullptr;
  } else {
    selectedWindow = FindWindowA(NULL, windows[index-1].c_str());
  }
  g_mutex_unlock(&shared_mutex);
}

void activate (GtkApplication *app,
               gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *button;
  GtkWidget *button2;

  // Create a new window, and set its title
  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Window");
  gtk_window_set_default_size (GTK_WINDOW (window), 400, 200);

  // list screens and windows
  EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));

  GtkWidget *comboBox = gtk_combo_box_text_new();
  // set combobox default option
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboBox), "Select a window");
  gtk_combo_box_set_active(GTK_COMBO_BOX(comboBox), 0);
  for(auto& windowTitle : windows) {
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboBox), windowTitle.c_str());
  }
  g_signal_connect (comboBox, "changed", G_CALLBACK (window_selected), NULL);
  
  button = gtk_button_new_with_label ("Start Recording");
  gtk_widget_set_margin_top (button, 10);
  g_signal_connect (button, "clicked", G_CALLBACK (start_recorder_thread), NULL);

  button2 = gtk_button_new_with_label ("Stop Recording");
  gtk_widget_set_margin_top (button2, 10);
  g_signal_connect (button2, "clicked", G_CALLBACK (stop_recorder), NULL);

  // combine items in grid
  GtkWidget *grid = gtk_grid_new ();
  gtk_grid_attach (GTK_GRID (grid), comboBox, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), button, 0, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), button2, 0, 2, 1, 1);
  gtk_window_set_child (GTK_WINDOW (window), grid);
  
  // Display the window
  gtk_widget_show (window);
}

int main (int   argc,
          char *argv[])
{
  // init gtk
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  // Wait for the thread to finish
  g_thread_join(recorder_thread);

  return status;
}

