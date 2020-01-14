#include <gtkmm.h>

class ModelColumns : public Gtk::TreeModelColumnRecord
{
public:
	ModelColumns();
	Gtk::TreeModelColumn<Glib::ustring> m_col_uid;
	Gtk::TreeModelColumn<Glib::ustring> m_col_key;
	Gtk::TreeModelColumn<Glib::ustring> m_col_value;
};

class MainWindow : public Gtk::Window
{
	public:
		MainWindow();

	protected:
		Glib::RefPtr<Gtk::TreeStore> ref_m_TreeStore;
		Gtk::ScrolledWindow view_ScrolledWindow;
		Gtk::TreeView view_TreeStore;
		void on_treeview_row_collapsed(const Gtk::TreeModel::iterator&, const Gtk::TreeModel::Path&);
		void on_treeview_row_expanded(const Gtk::TreeModel::iterator&, const Gtk::TreeModel::Path&);
		ModelColumns m_cols;
};

ModelColumns::ModelColumns()
{
	add(m_col_uid);
	add(m_col_key);
	add(m_col_value);
}

MainWindow::MainWindow()
{
	ref_m_TreeStore = Gtk::TreeStore::create(m_cols);

	Gtk::TreeModel::iterator it      = ref_m_TreeStore->append();
	Gtk::TreeModel::Row      m_row   = *it;
	m_row[m_cols.m_col_uid]          = "101";
	m_row[m_cols.m_col_key]          = "";
	m_row[m_cols.m_col_value]        = "Hello TreeStore!";

	Gtk::TreeModel::Row m_row_child1 = *(ref_m_TreeStore->append(m_row.children()));
	m_row_child1[m_cols.m_col_uid]   = "";
	m_row_child1[m_cols.m_col_key]   = "From";
	m_row_child1[m_cols.m_col_value] = "ximia greecia <ximia.greecia@one-start.tech>";

	Gtk::TreeModel::Row m_row_child2 = *(ref_m_TreeStore->append(m_row.children()));
	m_row_child2[m_cols.m_col_uid]   = "";
	m_row_child2[m_cols.m_col_key]   = "Date";
	m_row_child2[m_cols.m_col_value] = "Mon Aug 27, 2018 19:36:45 +0900 (JST)";

	view_TreeStore.signal_row_collapsed().connect(sigc::mem_fun(*this, &MainWindow::on_treeview_row_collapsed) );
	view_TreeStore.signal_row_expanded().connect(sigc::mem_fun(*this, &MainWindow::on_treeview_row_expanded) );

	view_TreeStore.set_model(ref_m_TreeStore);
	view_TreeStore.append_column("Key", m_cols.m_col_key);
	view_TreeStore.append_column("Value", m_cols.m_col_value);

	view_TreeStore.columns_autosize();
	view_ScrolledWindow.add(view_TreeStore);

	set_border_width(10);
	set_default_size(600, 250);
	add(view_ScrolledWindow);
	show_all_children();
}

void MainWindow::on_treeview_row_collapsed(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path)
{
  Gtk::TreeModel::iterator itr = ref_m_TreeStore->get_iter(path);
  if(itr)
  {
    Gtk::TreeModel::Row m_row = *itr;
    m_row[m_cols.m_col_key] = "";
	view_TreeStore.columns_autosize();
  }
}

void MainWindow::on_treeview_row_expanded(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path)
{
  Gtk::TreeModel::iterator itr = ref_m_TreeStore->get_iter(path);
  if(itr)
  {
    Gtk::TreeModel::Row m_row = *itr;
    m_row[m_cols.m_col_key] = "Title";
	view_TreeStore.columns_autosize();
  }
}

int main (int argc, char *argv[])
{

  auto app = Gtk::Application::create(argc, argv, "tech.one-start.examples");

  MainWindow mw;
  return app->run(mw);

}
