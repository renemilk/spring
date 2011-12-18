/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GUI_H
#define GUI_H

#include <QDialog>
#include <QTextEdit>
#include <QList>
#include <QBoxLayout>
class QGLContext;
class QDeclarativeContext;
class QDeclarativeView;
class QGLWidget;
union SDL_Event;

class QErrorWindow : public QDialog
{
    Q_OBJECT
public:
    template <class T>
    explicit QErrorWindow(const QList<T>& errors, QWidget *parent = 0);
};

namespace {
    template <class T>
    void appendErrorToTextEdit( const T& error, QTextEdit* text )
    {
        text->append( error.toString() );
    }

    template <>
    void appendErrorToTextEdit( const QString& error, QTextEdit* text )
    {
        QString f = error;
        text->append( f );
    }
}

#include <list>
#include "System/signalslib.h"
#include <boost/signals/connection.hpp>

namespace agui
{

class GuiElement;

class Gui
{
public:
	Gui();
	
	void Draw();
	void AddElement(GuiElement*, bool asBackground = false);
	/// deletes the element on the next draw
	void RmElement(GuiElement*);
	
	void UpdateScreenGeometry(int screenx, int screeny, int screenOffsetX, int screenOffsetY);

	bool MouseOverElement(const GuiElement*, int x, int y) const;

private:
	bool HandleEvent(const SDL_Event& ev);
    boost::signalslib::scoped_connection inputCon;

	struct GuiItem
	{
		GuiItem(GuiElement* el, bool back) : element(el), asBackground(back) {};
		GuiElement* element;
		bool asBackground;
	};
	typedef std::list<GuiItem> ElList;
	ElList elements;
	ElList toBeRemoved;
	ElList toBeAdded;
    QGLWidget* glWidget_;
    QDeclarativeView* view_;
    QDeclarativeContext* ctxt_;
    QGLContext* gl_ctxt_;
};

extern Gui* gui;
void InitGui();

}

#endif
