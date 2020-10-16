///////////////////////////////////////////////////////////////////////////////
// Name:        tests/controls/dataviewctrltest.cpp
// Purpose:     wxDataViewCtrl unit test
// Author:      Vaclav Slavik
// Created:     2011-08-08
// Copyright:   (c) 2011 Vaclav Slavik <vslavik@gmail.com>
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "testprec.h"

#if wxUSE_DATAVIEWCTRL

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/app.h"
#include "wx/dataview.h"
#include "wx/sizer.h"
#include "wx/dialog.h"
#ifdef __WXGTK__
    #include "wx/stopwatch.h"
#endif // __WXGTK__

#include "testableframe.h"
#include "asserthelper.h"

// ----------------------------------------------------------------------------
// test class
// ----------------------------------------------------------------------------

class DataViewCtrlTestCase
{
public:
    explicit DataViewCtrlTestCase(long style);
    ~DataViewCtrlTestCase();

protected:
    void TestSelectionFor0and1();

    // the dataview control itself
    wxDataViewTreeCtrl *m_dvc;

    // and some of its items
    wxDataViewItem m_root,
                   m_child1,
                   m_child2,
                   m_grandchild;

    wxDECLARE_NO_COPY_CLASS(DataViewCtrlTestCase);
};

class SingleSelectDataViewCtrlTestCase : public DataViewCtrlTestCase
{
public:
    SingleSelectDataViewCtrlTestCase()
        : DataViewCtrlTestCase(wxDV_SINGLE)
    {
    }
};

class MultiSelectDataViewCtrlTestCase : public DataViewCtrlTestCase
{
public:
    MultiSelectDataViewCtrlTestCase()
        : DataViewCtrlTestCase(wxDV_MULTIPLE)
    {
    }
};

class MultiColumnsDataViewCtrlTestCase
{
public:
    MultiColumnsDataViewCtrlTestCase();
    ~MultiColumnsDataViewCtrlTestCase();

protected:
    // the dataview control itself
    wxDataViewListCtrl *m_dvc;

    // constants
    const wxSize m_size;
    const int m_firstColumnWidth;

    // and the columns
    wxDataViewColumn* m_firstColumn;
    wxDataViewColumn* m_lastColumn;

    wxDECLARE_NO_COPY_CLASS(MultiColumnsDataViewCtrlTestCase);
};

class DataViewCtrlWithModelTestCase
{
public:
    DataViewCtrlWithModelTestCase();

protected:
    wxDialog* m_dialog;
    // the dataview control itself
    wxDataViewCtrl* m_dvc;

    wxDataViewColumn* m_firstColumn;
    wxDataViewColumn* m_lastColumn;

    wxDECLARE_NO_COPY_CLASS(DataViewCtrlWithModelTestCase);
};

// ----------------------------------------------------------------------------
// test initialization
// ----------------------------------------------------------------------------

DataViewCtrlTestCase::DataViewCtrlTestCase(long style)
{
    m_dvc = new wxDataViewTreeCtrl(wxTheApp->GetTopWindow(),
                                   wxID_ANY,
                                   wxDefaultPosition,
                                   wxSize(400, 200),
                                   style);

    m_root = m_dvc->AppendContainer(wxDataViewItem(), "The root");
      m_child1 = m_dvc->AppendContainer(m_root, "child1");
        m_grandchild = m_dvc->AppendItem(m_child1, "grandchild");
      m_child2 = m_dvc->AppendItem(m_root, "child2");

    m_dvc->Layout();
    m_dvc->Expand(m_root);
    m_dvc->Refresh();
    m_dvc->Update();
}

DataViewCtrlTestCase::~DataViewCtrlTestCase()
{
    delete m_dvc;
}

MultiColumnsDataViewCtrlTestCase::MultiColumnsDataViewCtrlTestCase()
    : m_size(200, 100),
      m_firstColumnWidth(50)
{
    m_dvc = new wxDataViewListCtrl(wxTheApp->GetTopWindow(), wxID_ANY);

    m_firstColumn =
        m_dvc->AppendTextColumn(wxString(), wxDATAVIEW_CELL_INERT, m_firstColumnWidth);
    m_lastColumn =
        m_dvc->AppendTextColumn(wxString(), wxDATAVIEW_CELL_INERT);

    // Set size after columns appending to extend size of the last column.
    m_dvc->SetSize(m_size);
    m_dvc->Layout();
    m_dvc->Refresh();
    m_dvc->Update();
}

MultiColumnsDataViewCtrlTestCase::~MultiColumnsDataViewCtrlTestCase()
{
    delete m_dvc;
}

// Custom wxDataViewModel implementation
class CustomTreeModel :
    public wxDataViewModel
{
public:
    struct Node
    {
        Node(Node* parent_, const wxString& value_) :
            parent(parent_),
            value(value_)
        {}

        ~Node()
        {
            for (auto* child : children)
            {
                delete child;
            }
        }

        Node(const Node& other) = default;
        Node& operator=(const Node& other) = default;

        Node* AddChild(const wxString& childValue)
        {
            children.push_back(new Node(this, childValue));
            return children.back();
        }

        Node* parent;
        wxVariant value;
        wxVector<Node*> children;
    };

private:
    Node* m_root;
public:
    CustomTreeModel() :
        m_root(new Node(nullptr, "Root"))
    {}

    Node* GetRoot()
    {
        return m_root;
    }

    void Populate()
    {
        // Build the test structure, just a few items
        auto* a = m_root->AddChild("A");

        auto* aa = a->AddChild("A-A");
        auto* ab = a->AddChild("A-B");
        auto* ac = a->AddChild("A-C");

        aa->AddChild("A-A-1");
        ab->AddChild("A-B-1");
        ac->AddChild("A-C-2");
    }

    ~CustomTreeModel()
    {
        delete m_root;
    }

    unsigned int GetColumnCount() const override
    {
        return 1;
    }

    wxString GetColumnType(unsigned int WXUNUSED(col)) const override
    {
        return "string";
    }

    void GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int WXUNUSED(col)) const override
    {
        const Node* node = item.IsOk() ? static_cast<const Node*>(item.GetID()) : m_root;
        variant = node->value;
    }

    bool SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int WXUNUSED(col)) override
    {
        Node* node = item.IsOk() ? static_cast<Node*>(item.GetID()) : m_root;
        node->value = variant;
        return true;
    }

    wxDataViewItem GetParent(const wxDataViewItem& item) const override
    {
        if (!item.IsOk())
        {
            return wxDataViewItem(nullptr);
        }

        Node* node = static_cast<Node*>(item.GetID());

        if (node->parent != nullptr && node->parent != m_root)
        {
            return wxDataViewItem(node->parent);
        }

        return wxDataViewItem(nullptr);
    }

    bool IsContainer(const wxDataViewItem& WXUNUSED(item)) const override
    {
        return true; // all nodes can be containers
    }

    unsigned int GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const override
    {
        // Requests for invalid items are asking for root children, actually
        const Node* node = !item.IsOk() ? m_root : static_cast<const Node*>(item.GetID());

        for (auto* child : node->children)
        {
            children.Add(wxDataViewItem(child));
        }

        return static_cast<unsigned int>(node->children.size());
    }
};

void SendItemAddedEventsForAllChildrenRecursively(CustomTreeModel& model, CustomTreeModel::Node* node)
{
    for (auto* child : node->children)
    {
        wxDataViewItem item(child);
        model.ItemAdded(model.GetParent(item), item);

        SendItemAddedEventsForAllChildrenRecursively(model, child);
    }
}

DataViewCtrlWithModelTestCase::DataViewCtrlWithModelTestCase()
{
    m_dialog = new wxDialog(wxTheApp->GetTopWindow(), wxID_ANY, "DataViewCtrlWithModel");
    m_dialog->SetSizer(new wxBoxSizer(wxHORIZONTAL));

    m_dvc = new wxDataViewCtrl(m_dialog, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    m_dialog->GetSizer()->Add(m_dvc, 1, wxEXPAND|wxALL, 6);

    // Create the test data model
    auto* model = new CustomTreeModel();

    m_dvc->AssociateModel(model);
    model->DecRef(); // pass ownership to dataviewctrl

    m_firstColumn = m_dvc->AppendTextColumn(wxString(), 0, wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);
}

// ----------------------------------------------------------------------------
// the tests themselves
// ----------------------------------------------------------------------------

TEST_CASE_METHOD(DataViewCtrlWithModelTestCase, "wxDataViewModel::ItemAdded")
{
    auto* model = static_cast<CustomTreeModel*>(m_dvc->GetModel());

    // Pump some data into the model and send ItemAdded events
    model->Populate();

    // After sending the ItemAdded events, all the items
    // below the first hierarchy level end up with duplicate representations
    // in the tree.
    SendItemAddedEventsForAllChildrenRecursively(*model, model->GetRoot());

    // No assert here, since we don't have access to the underlying wxDataViewMainWindow
    // where all the duplicated nodes are created and added to
    m_dialog->Layout();
    m_dialog->ShowModal();
    m_dialog->Destroy();
}

TEST_CASE_METHOD(MultiSelectDataViewCtrlTestCase,
                 "wxDVC::Selection",
                 "[wxDataViewCtrl][select]")
{
    // Check selection round-trip.
    wxDataViewItemArray sel;
    sel.push_back(m_child1);
    sel.push_back(m_grandchild);
    REQUIRE_NOTHROW( m_dvc->SetSelections(sel) );

    wxDataViewItemArray sel2;
    CHECK( m_dvc->GetSelections(sel2) == sel.size() );

    CHECK( sel2 == sel );

    // Invalid items in GetSelections() input are supposed to be just skipped.
    sel.clear();
    sel.push_back(wxDataViewItem());
    REQUIRE_NOTHROW( m_dvc->SetSelections(sel) );

    CHECK( m_dvc->GetSelections(sel2) == 0 );
    CHECK( sel2.empty() );
}

TEST_CASE_METHOD(MultiSelectDataViewCtrlTestCase,
                 "wxDVC::DeleteSelected",
                 "[wxDataViewCtrl][delete]")
{
    wxDataViewItemArray sel;
    sel.push_back(m_child1);
    sel.push_back(m_grandchild);
    sel.push_back(m_child2);
    m_dvc->SetSelections(sel);

    // delete a selected item
    m_dvc->DeleteItem(m_child1);

    m_dvc->GetSelections(sel);

    // m_child1 and its children should be removed from the selection now
    REQUIRE( sel.size() == 1 );
    CHECK( sel[0] == m_child2 );
}

TEST_CASE_METHOD(MultiSelectDataViewCtrlTestCase,
                 "wxDVC::DeleteNotSelected",
                 "[wxDataViewCtrl][delete]")
{
    // TODO not working on OS X as expected
#ifdef __WXOSX__
    WARN("Disabled under MacOS because this test currently fails");
#else
    wxDataViewItemArray sel;
    sel.push_back(m_child1);
    sel.push_back(m_grandchild);
    m_dvc->SetSelections(sel);

    // delete unselected item
    m_dvc->DeleteItem(m_child2);

    m_dvc->GetSelections(sel);

    // m_child1 and its children should be unaffected
    REQUIRE( sel.size() == 2 );
    CHECK( sel[0] == m_child1 );
    CHECK( sel[1] == m_grandchild );
#endif
}

void DataViewCtrlTestCase::TestSelectionFor0and1()
{
    wxDataViewItemArray selections;

    // Initially there is no selection.
    CHECK( m_dvc->GetSelectedItemsCount() == 0 );
    CHECK( !m_dvc->HasSelection() );
    CHECK( !m_dvc->GetSelection().IsOk() );

    CHECK( !m_dvc->GetSelections(selections) );
    CHECK( selections.empty() );

    // Select one item.
    m_dvc->Select(m_child1);
    CHECK( m_dvc->GetSelectedItemsCount() == 1 );
    CHECK( m_dvc->HasSelection() );
    CHECK( m_dvc->GetSelection().IsOk() );
    REQUIRE( m_dvc->GetSelections(selections) == 1 );
    CHECK( selections[0] == m_child1 );
}

TEST_CASE_METHOD(MultiSelectDataViewCtrlTestCase,
                 "wxDVC::GetSelectionForMulti",
                 "[wxDataViewCtrl][select]")
{
    wxDataViewItemArray selections;

    TestSelectionFor0and1();

    m_dvc->Select(m_child2);

    CHECK( m_dvc->GetSelectedItemsCount() == 2 );
    CHECK( m_dvc->HasSelection() );
    CHECK( !m_dvc->GetSelection().IsOk() );
    REQUIRE( m_dvc->GetSelections(selections) == 2 );
    CHECK( selections[1] == m_child2 );
}

TEST_CASE_METHOD(SingleSelectDataViewCtrlTestCase,
                 "wxDVC::SingleSelection",
                 "[wxDataViewCtrl][selection]")
{
    TestSelectionFor0and1();
}

TEST_CASE_METHOD(SingleSelectDataViewCtrlTestCase,
                 "wxDVC::IsExpanded",
                 "[wxDataViewCtrl][expand]")
{
    CHECK( m_dvc->IsExpanded(m_root) );
    CHECK( !m_dvc->IsExpanded(m_child1) );
    // No idea why, but the native NSOutlineView isItemExpanded: method returns
    // true for this item for some reason.
#ifdef __WXOSX__
    WARN("Disabled under MacOS: IsExpanded() returns true for grand child");
#else
    CHECK( !m_dvc->IsExpanded(m_grandchild) );
#endif
    CHECK( !m_dvc->IsExpanded(m_child2) );
}

TEST_CASE_METHOD(SingleSelectDataViewCtrlTestCase,
                 "wxDVC::GetItemRect",
                 "[wxDataViewCtrl][item]")
{
#ifdef __WXGTK__
    // We need to let the native control have some events to lay itself out.
    wxYield();
#endif // __WXGTK__

    const wxRect rect1 = m_dvc->GetItemRect(m_child1);
    const wxRect rect2 = m_dvc->GetItemRect(m_child2);

    CHECK( rect1 != wxRect() );
    CHECK( rect2 != wxRect() );

    CHECK( rect1.x == rect2.x );
    CHECK( rect1.width == rect2.width );
    CHECK( rect1.height == rect2.height );

    {
        INFO("First child: " << rect1 << ", second one: " << rect2);
        CHECK( rect1.y < rect2.y );
    }

    const wxRect rectNotShown = m_dvc->GetItemRect(m_grandchild);
    CHECK( rectNotShown == wxRect() );

    // Append enough items to make the window scrollable.
    for ( int i = 3; i < 100; ++i )
        m_dvc->AppendItem(m_root, wxString::Format("child%d", i));

    const wxDataViewItem last = m_dvc->AppendItem(m_root, "last");

    // This should scroll the window to bring this item into view.
    m_dvc->EnsureVisible(last);

#ifdef __WXGTK__
    // And again to let it scroll the correct items into view.
    wxYield();
#endif

    // Check that this was indeed the case.
    const wxDataViewItem top = m_dvc->GetTopItem();
    CHECK( top != m_root );

    // Verify that the coordinates are returned in physical coordinates of the
    // window and not the logical coordinates affected by scrolling.
    const wxRect rectScrolled = m_dvc->GetItemRect(top);
    CHECK( rectScrolled.GetBottom() > 0 );
    CHECK( rectScrolled.GetTop() <= m_dvc->GetClientSize().y );

    // Also check that the root item is not currently visible (because it's
    // scrolled off).
    const wxRect rectRoot = m_dvc->GetItemRect(m_root);
    CHECK( rectRoot == wxRect() );
}

TEST_CASE_METHOD(SingleSelectDataViewCtrlTestCase,
                 "wxDVC::DeleteAllItems",
                 "[wxDataViewCtrl][delete]")
{
    // The invalid item corresponds to the root of tree store model, so it
    // should have a single item (our m_root) initially.
    CHECK( m_dvc->GetChildCount(wxDataViewItem()) == 1 );

    m_dvc->DeleteAllItems();

    // And none at all after deleting all the items.
    CHECK( m_dvc->GetChildCount(wxDataViewItem()) == 0 );
}

TEST_CASE_METHOD(MultiColumnsDataViewCtrlTestCase,
                 "wxDVC::AppendTextColumn",
                 "[wxDataViewCtrl][column]")
{
#ifdef __WXGTK__
    // Wait for the list control to be realized.
    wxStopWatch sw;
    while ( m_firstColumn->GetWidth() == 0 )
    {
        if ( sw.Time() > 500 )
        {
            WARN("Timed out waiting for wxDataViewListCtrl to be realized");
            break;
        }
        wxYield();
    }
#endif

    // Check the width of the first column.
    CHECK( m_firstColumn->GetWidth() == m_firstColumnWidth );

    // Check that the last column was extended to fit client area.
    const int lastColumnMaxWidth =
        m_dvc->GetClientSize().GetWidth() - m_firstColumnWidth;
    // In GTK and under Mac the width of the last column is less then
    // a remaining client area.
    const int lastColumnMinWidth = lastColumnMaxWidth - 10;
    CHECK( m_lastColumn->GetWidth() <= lastColumnMaxWidth );
    CHECK( m_lastColumn->GetWidth() >= lastColumnMinWidth );
}

#endif //wxUSE_DATAVIEWCTRL
