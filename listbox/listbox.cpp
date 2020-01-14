#include <gtkmm.h>

class ExampleWindow : public Gtk::Window
{
public:
  ExampleWindow();
  virtual ~ExampleWindow();

protected:

  //Child widgets:
  Gtk::Frame m_Frame;
  Gtk::ListBox m_ListBox;
  Gtk::Label m_Label1;
  Gtk::Label m_Label2;
  Gtk::Label m_Label3;
};

ExampleWindow::ExampleWindow()
{
 /* Set some window properties */
  set_title("Frame Example");
  set_size_request(300, 300);

  /* Sets the border width of the window. */
  set_border_width(10);

  add(m_Frame);

  /* Set the frames label */
  m_Frame.set_label("Gtk::Frame Widget");
  m_Frame.set_shadow_type(Gtk::SHADOW_ETCHED_OUT);

  m_Frame.add(m_ListBox);

  m_Label1.set_label("Hello!");
  m_Label2.set_label("Hi!");
  m_Label3.set_label("Hey!");

  m_ListBox.append(m_Label1);
  m_ListBox.append(m_Label2);
  m_ListBox.append(m_Label3);

  /* Align the label at the right of the frame */
  //m_Frame.set_label_align(Gtk::ALIGN_END, Gtk::ALIGN_START);

  /* Set the style of the frame */

  show_all_children();
}

ExampleWindow::~ExampleWindow()
{
}

int main(int argc, char *argv[])
{
  auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

  ExampleWindow window;

  //Shows the window and returns when it is closed.
  return app->run(window);
}
