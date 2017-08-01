//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2015-2017.
//  Released under the MIT Software license; see doc/LICENSE
//  Formerly: Slate.h
//
//
//  Div Widgets maintain an ordered collection of other widgets.  When printed to the
//  web page, these internal widgets are presented in order.
//
//  To create a Div:
//
//    emp::web::Div my_div("name");
//
//  To use a Div:
//
//    my_div << "Add this text!"
//           << emp::web::Image("my_image.png")
//           << "<br>";
//
//  To register a Div in a Document:
//
//    my_doc << my_div;
//
//
//  Development Notes:
//  * Should we move all widget_dict info into Document?

#ifndef EMP_WEB_DIV_H
#define EMP_WEB_DIV_H


#include "Text.h"
#include "Widget.h"

namespace emp {
namespace web {

  class Button;
  class Canvas;
  class Image;
  class Selector;
  class Div;
  class Table;

  namespace internal {

    class TableInfo;
    class DivInfo : public internal::WidgetInfo {
      friend Div; friend TableInfo;
    protected:
      double scroll_top;                         // Where should div scroll to? (0.0 to 1.0)
      emp::vector<Widget> m_children;            // Widgets contained in this one.
      bool append_ok;                            // Can we add more children?
      std::map<std::string, Widget> widget_dict; // By-name lookup for descendent widgets


      DivInfo(const std::string & in_id="")
        : internal::WidgetInfo(in_id), scroll_top(0.0), append_ok(true) { ; }
      DivInfo(const DivInfo &) = delete;               // No copies of INFO allowed
      DivInfo & operator=(const DivInfo &) = delete;   // No copies of INFO allowed
      virtual ~DivInfo() { ; }

      std::string TypeName() const override { return "DivInfo"; }

      virtual bool IsDivInfo() const override { return true; }


      bool IsRegistered(const std::string & test_name) const {
        return (widget_dict.find(test_name) != widget_dict.end());
      }

      Widget & GetRegistered(const std::string & find_name) {
        emp_assert(IsRegistered(find_name), find_name, widget_dict.size());
        return widget_dict[find_name];
      }

      void Register_recurse(Widget & new_widget) override {
        emp_assert(IsRegistered(new_widget.GetID()) == false, new_widget.GetID());
        widget_dict[new_widget.GetID()] = new_widget;     // Track widget by name
        if (parent) parent->Register_recurse(new_widget); // Also register in parent, if available
      }

      // Register is used so we can lookup classes by name.
      void Register(Widget & new_widget) override {
        Register_recurse(new_widget);          // Register THIS widget here an in ancestors.
        new_widget->RegisterChildren( this );  // Register CHILD widgets, if any
      }

      void RegisterChildren(DivInfo * regestrar) override {
        for (Widget & child : m_children) regestrar->Register(child);
      }

      void Unregister_recurse(Widget & old_widget) override {
        emp_assert(IsRegistered(old_widget.GetID()) == true, old_widget.GetID());
        widget_dict.erase(old_widget.GetID());
        if (parent) parent->Unregister_recurse(old_widget); // Unregister in parent, if available
      }

      void Unregister(Widget & old_widget) override {
        Unregister_recurse(old_widget);          // Unregister this node from all above.
        old_widget->UnregisterChildren( this );  // Unregister all children, if any.
        old_widget->parent = nullptr;
        old_widget.Deactivate(false);
      }

      void UnregisterChildren(DivInfo * regestrar) override {
        for (Widget & child : m_children) regestrar->Unregister(child);
      }

      void ClearChildren() {
        // Unregister all children and then delete links to them.
        for (Widget & child : m_children) Unregister(child);
        m_children.resize(0);
        if (state == Widget::ACTIVE) ReplaceHTML();
      }

      void Clear() {
        ClearChildren();
        extras.Clear();
        if (state == Widget::ACTIVE) ReplaceHTML();
      }

      void AddChild(Widget in) {
        // If the inserted widget is already active, remove it from its old position.
        emp_assert(in->parent == nullptr && "Cannot insert widget if already has parent!", in->id);
        emp_assert(in->state != Widget::ACTIVE && "Cannot insert a stand-alone active widget!");

        // Setup parent-child relationship
        m_children.emplace_back(in);
        in->parent = this;
        Register(in);

        // If this element (as new parent) is active, anchor widget and activate it!
        if (state == Widget::ACTIVE) {
          // Create a span tag to anchor the new widget.
          EM_ASM_ARGS({
              parent_id = Pointer_stringify($0);
              child_id = Pointer_stringify($1);
              $('#' + parent_id).append('<span id="' + child_id + '"></span>');
            }, id.c_str(), in.GetID().c_str());

          // Now that the new widget has some place to hook in, activate it!
          in->DoActivate();
        }
      }

      void DoActivate(bool top_level=true) override {
        for (auto & child : m_children) child->DoActivate(false);
        internal::WidgetInfo::DoActivate(top_level);
      }


      // Return a text element for appending.  Use the last element unless there are no elements,
      // the last element is not text, or it is not appendable (instead, build a new one).
      web::Text & GetTextWidget() {
        // If the final element is not text, add one.
        if (m_children.size() == 0
            || m_children.back().IsText() == false
            || m_children.back().AppendOK() == false)  {
          AddChild(Text());
        }
        return (Text &) m_children.back();
      }

      bool AppendOK() const override { return append_ok; }
      void PreventAppend() override { append_ok = false; }

      // Add additional children on to this element.
      Widget Append(const std::string & text) override {
        if (!append_ok) return ForwardAppend(text);
        return GetTextWidget() << text;
      }
      Widget Append(const std::function<std::string()> & in_fun) override {
        if (!append_ok) return ForwardAppend(in_fun);
        return GetTextWidget() << in_fun;
      }

      Widget Append(Widget info) override {
        if (!append_ok) return ForwardAppend(info);
        AddChild(info);
        return info;
      }

      // All derived widgets must suply a mechanism for providing associated HTML code.
      virtual void GetHTML(std::stringstream & HTML) override {
        HTML.str("");       // Clear the current text.

        // Loop through all children and build a span element for each to replace.
        HTML << "<div id=\'" << id << "\'>"; // Tag to envelope Div
        for (Widget & w : m_children) {
          HTML << "<span id=\'" << w.GetID() << "'></span>";  // Span element for current widget.
        }
        HTML << "</div>";
      }


      void ReplaceHTML() override {
        // Replace Div's HTML...
        internal::WidgetInfo::ReplaceHTML();

        // Then replace children.
        if (state == Widget::ACTIVE) {
          for (auto & child : m_children) child->ReplaceHTML();
        }

        if (scroll_top >= 0.0) {
          EM_ASM_ARGS({
              var div_id = Pointer_stringify($0);
              var div_obj = document.getElementById(div_id);
              if (div_obj == null) alert(div_id);
              // alert('id=' + div_id + '  top=' + $1 +
              //       '  height=' + div_obj.scrollHeight);
              var scroll_top = $1 * div_obj.scrollHeight;
              div_obj.scrollTop = scroll_top;
            }, id.c_str(), scroll_top);
        }

        // @CAO If scrolltop is set, handle scrolling!
    // float scroll_frac = ((float) (hardware->GetIP() - 3)) / (float) hardware->GetNumInsts();
    // if (scroll_frac < 0.0) scroll_frac = 0.0;

    // EM_ASM_ARGS({
    //     var code = Pointer_stringify($0);
    //     var code_obj = document.getElementById("code");
    //     code_obj.innerHTML = code;
    //     code_obj.scrollTop = $1 * code_obj.scrollHeight;
    //     var cycle_obj = document.getElementById("cycle_count");
    //     cycle_obj.innerHTML = "&nbsp;&nbsp;&nbsp;Cycles Used = " + $2;
    // }, ss.str().c_str(), scroll_frac, hardware->GetExeCount());

      }

    public:
      virtual std::string GetType() override { return "web::DivInfo"; }
    };
  }

  class Div : public internal::WidgetFacet<Div> {
  protected:
    // Get a properly cast version of indo.
    internal::DivInfo * Info() { return (internal::DivInfo *) info; }
    const internal::DivInfo * Info() const { return (internal::DivInfo *) info; }

  public:
    Div(const std::string & in_name) : WidgetFacet(in_name) {
      // When a name is provided, create an associated Widget info.
      info = new internal::DivInfo(in_name);
    }
    Div(const Div & in) : WidgetFacet(in) { ; }
    Div(const Widget & in) : WidgetFacet(in) { emp_assert(info->IsDivInfo()); }
    Div() { ; }
    ~Div() { ; }

    using INFO_TYPE = internal::DivInfo;

    double ScrollTop() const { return Info()->scroll_top; }
    Div & ScrollTop(double in_top) { Info()->scroll_top = in_top; return *this; }

    void Clear() { if (info) Info()->Clear(); }
    void ClearChildren() { if (info) Info()->ClearChildren(); }

    bool HasChild(const Widget & test_child) const {
      if (!info) return false;
      for (const Widget & c : Info()->m_children) if (c == test_child) return true;
      return false;
    }

    void Deactivate(bool top_level) override {
      // Deactivate children before this node.
      for (auto & child : Info()->m_children) child.Deactivate(false);
      Widget::Deactivate(top_level);
    }

    Widget & Find(const std::string & test_name) {
      emp_assert(info);
      return Info()->GetRegistered(test_name);
    }

    emp::vector<Widget> & Children() { return Info()->m_children; }

  };

  // using Slate = Div;    // For backward compatability...
}
}

#endif
