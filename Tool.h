#ifndef TOOL_H
#define TOOL_H

#include <gtk/gtk.h>
#include "image_info.h"

/** Base class for all tools. */
class Tool
{
public:
    Tool(image_info* img);

    virtual ~Tool() {}

    /** Activate the tool (user clicked on the toolbar icon for this
        tool). Return value: if false, instantly de-activate the tool
        (tools that do their work instantly, like zoom out). */
    virtual bool activate() = 0;

    /** Deactivate the tool. Reset any internal state, remove timers,
        redraw screen to get rid of anything the tool might have painted,
        etc. */
    virtual void deactivate() { }

    /** Types of mouse button clicks. */
    enum ButtonType { LEFT, MIDDLE, RIGHT };

    /** Convert from GDK's button type to ours. */
    static ButtonType fromGDKButtonType(int button);

    /** Mouse button event. */
    virtual void buttonEvent(ButtonType type, bool isPress, int x, int y) { }

    /** Mouse movement event. */
    virtual void moveEvent(int x, int y) { }

    /** Keyboard event. If return value is true, the event was consumed by
        the tool. */
    virtual bool keyEvent(GdkEventKey* ev) { return false; }

    /** Size of the image has changed. */
    virtual void sizeEvent(bool toolIsActive) { }

protected:
    image_info* img;
};

/** Dummy tool, does absolutely nothing. Useful if you want to have a
    pointer to a tool that can sometimes point to no real tool and call
    functions on that pointer without checking if it's NULL all the
    time. */
class DummyTool : public Tool
{
public:
    DummyTool() : Tool(NULL) { }

    virtual bool activate() { return false; }
};

/** Zoom out. */
class ZoomOutTool : public Tool
{
public:
    ZoomOutTool(image_info* img);

    virtual bool activate();
};

/** Zoom in. */
class ZoomInTool : public Tool
{
public:
    ZoomInTool(image_info* img, GtkWidget* toolbarButton);

    virtual bool activate();
    virtual void deactivate();
    virtual void buttonEvent(ButtonType type, bool isPress, int x, int y);
    virtual void moveEvent(int x, int y);
    virtual bool keyEvent(GdkEventKey* ev);

    void timerCallback(int arg);

    /** Needed for the timer callback. */
    static ZoomInTool* me;

private:
    // zoom in to the selected area
    void zoom();

    void killTimer();

    void resizeRect(int arg);

    int rectIsValidSize();

    // ms between zoom timer callbacks
    static const int ZOOM_INTERVAL = 25;

    // percentage of image width that zoom box starts at */
    static const float ZOOM_BOX_WIDTH;

    int timer_id;

    GtkWidget* toolbarButton;

    // zoom box coordinates
    GdkRectangle rect;
};

/** Crop. */
class CropTool : public Tool
{
public:
    CropTool(image_info* img, GtkWidget* toolbarButton);

    virtual bool activate();
    virtual void deactivate();
    virtual void buttonEvent(ButtonType type, bool isPress, int x, int y);
    virtual void moveEvent(int x, int y);

private:
    GdkRectangle getRect();

    void crop();

    GtkWidget* toolbarButton;

    // position of first click. x1 == -1 -> not drawn on screen yet
    int x1, y1;

    // changing position of cursor
    int x2, y2;
};

/** Switch between Mandelbrot/Julia modes. */
class JuliaTool : public Tool
{
public:
    JuliaTool(image_info* img, GtkWidget* toolbarButton);

    virtual bool activate();
    virtual void deactivate();
    virtual void buttonEvent(ButtonType type, bool isPress, int x, int y);
    virtual void moveEvent(int x, int y);
    virtual void sizeEvent(bool toolIsActive);

private:
    enum
    {
        JPRE_SIZE = 160,
        JPRE_AA_FACTOR = 2
    };

    GtkWidget* toolbarButton;

    // Julia preview info & window
    image_info j_pre;
    GtkWidget* j_pre_window;

    // Mandelbrot coordinates when we switched away from it
    double old_xmin, old_xmax, old_ymax;
};

#endif
