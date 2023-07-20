#include <gtk/gtk.h>
// #include <objbase.h>  // For CoInitializeEx(), COINIT_MULTITHREADED
#include "creator.cpp"

void activate (GtkApplication *app,
               gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *button;

  // Create a new window, and set its title
  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Window");
  
  // Here we construct the container that is going pack our button
  button = gtk_button_new_with_label ("Transform Video");

  // Connect the "clicked" signal of the button to our callback
  g_signal_connect (button, "clicked", G_CALLBACK (transform_video), NULL);
  
  // Add the button to the window
  gtk_window_set_child (GTK_WINDOW (window), button);

  // Display the window
  gtk_widget_show (window);
}

int main (int   argc,
          char *argv[])
{
  // multithreading for ffmpeg
  // HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  // if (FAILED(hr)) {
  //     fprintf(stderr, "CoInitializeEx failed: %08X\n", hr);
  //     return 1;
  // }

  // init gtk
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  // clean up
  // CoUninitialize();

  return status;
}

