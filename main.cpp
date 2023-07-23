#include <gtk/gtk.h>
#include "creator.cpp"
#include "recorder.cpp"

GThread* recorder_thread = nullptr;

void start_recorder_thread() {
  recorder_thread = g_thread_new("recorder_thread", start_recorder, nullptr);
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
  
  button = gtk_button_new_with_label ("Start Recording");
  // gtk_widget_set_margin_bottom (button, 100);
  g_signal_connect (button, "clicked", G_CALLBACK (start_recorder_thread), NULL);

  button2 = gtk_button_new_with_label ("Stop Recording");
  gtk_widget_set_margin_top (button2, 100);
  g_signal_connect (button2, "clicked", G_CALLBACK (stop_recorder), NULL);

  // combine buttons in grid
  GtkWidget *grid = gtk_grid_new ();
  gtk_grid_attach (GTK_GRID (grid), button, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), button2, 0, 1, 1, 1);
  gtk_window_set_child (GTK_WINDOW (window), grid);
  
  // gtk_window_set_child (GTK_WINDOW (window), button);
  // gtk_window_set_child (GTK_WINDOW (window), button2);
  // set multiple children

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

