/*
 * xdraw.c - Base for 1. project
 */

/*
 * Standard XToolkit and OSF/Motif include files.
 */
#include <X11/Intrinsic.h>
#include <Xm/Xm.h> 

/*
 * Public include files for widgets used in this file.
 */
#include <Xm/MainW.h> 
#include <Xm/Form.h> 
#include <Xm/Frame.h>
#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>

/*
 * Common C library include files
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
/*
 * Shared variables
 */

#define OBJECTS_ALLOC_STEP	10	/* memory allocation stepping */
XSegment *objects = NULL;		/* array of object descriptors */
int maxobjects = 0;		/* space allocated for max objects */
int nobjects = 0;			/* current number of objects */

GC drawGC = 0;			/* GC used for final drawing */
GC inputGC = 0;			/* GC used for drawing current position */

int x1, y1, x2, y2;		/* input points */ 
int rectX, rectY;
int button_pressed = 0;		/* input state */

int drawingMode = 0;
int lineWidth = 0;
int lineType = 0;

enum drawingModes {
	LINE = 0,
	POINT = 1,
	RECTANGLE = 2,
	ELLIPSE = 3
};

enum lineTypes {
	SOLID = LineSolid,
	DASHED = LineDoubleDash
};

Display *display;		
Colormap cmap;

Pixel drawAreaPixel = 0;
Pixel lineColorFgPixel = 0;	
Pixel lineColorBgPixel = 0;	
Pixel fillColorFgPixel = 0;	
Pixel fillColorBgPixel = 0;	

void countRectangleParameters(int x1, int y1, int x2, int y2) {

	if (x1 > x2) {
		rectX = x2;
	}
	else {
		rectX = x1;
	}

	if (y1 > y2) {
		rectY = y2;
	}
	else {
		rectY = y1;
	}

}


/*
 * "input" event handler
 */
/* ARGSUSED */
void inputEH(Widget w, XtPointer client_data, XEvent *event, Boolean *cont)
{
    Pixel	fg, bg;

    if (button_pressed) {
		if (!inputGC) {
		    inputGC = XCreateGC(XtDisplay(w), XtWindow(w), 0, NULL);
		    XSetFunction(XtDisplay(w), inputGC, GXxor);
		    XSetPlaneMask(XtDisplay(w), inputGC, ~0);
		    XtVaGetValues(w, XmNforeground, &fg, XmNbackground, &bg, NULL);
		    XSetForeground(XtDisplay(w), inputGC, fg ^ bg);
		}

		if (lineWidth == 0) {
			XSetLineAttributes(XtDisplay(w), inputGC, lineWidth, LineSolid, CapButt, JoinMiter);
		}
		else {
			XSetLineAttributes(XtDisplay(w), inputGC, lineWidth, lineType, CapButt, JoinMiter);
		}

		if (button_pressed > 1) {
		    /* erase previous position */
			XSetForeground(XtDisplay(w), inputGC, drawAreaPixel ^ lineColorFgPixel);

		    if (drawingMode == LINE) {
				XDrawLine(XtDisplay(w), XtWindow(w), inputGC, x1, y1, x2, y2);
		    }
		    else if (drawingMode == POINT) {
				// if (lineWidth != 0) {
				// 	XFillArc(XtDisplay(w), XtWindow(w), inputGC, x2 - (lineWidth/2), y2 - (lineWidth/2), lineWidth, lineWidth, 0, 360*64);
				// }	
				// else {
				// 	XDrawPoint(XtDisplay(w), XtWindow(w), inputGC, x2, y2);		
				// }
			}
			else if (drawingMode == RECTANGLE) {
				countRectangleParameters(x1, y1, x2, y2);
				XSetForeground(XtDisplay(w), inputGC, drawAreaPixel ^ fillColorFgPixel);
				XFillRectangle(XtDisplay(w), XtWindow(w), inputGC, rectX, rectY, abs(x1 - x2), abs(y1 - y2));
				XSetForeground(XtDisplay(w), inputGC, drawAreaPixel ^ lineColorFgPixel);
				XDrawRectangle(XtDisplay(w), XtWindow(w), inputGC, rectX, rectY, abs(x1 - x2), abs(y1 - y2));
			}

		} else {
		    /* remember first MotionNotify */
		    button_pressed = 2;
		}

		x2 = event->xmotion.x;
		y2 = event->xmotion.y;

		XSetForeground(XtDisplay(w), inputGC, drawAreaPixel ^ lineColorFgPixel);
		if (drawingMode == LINE) {
			XDrawLine(XtDisplay(w), XtWindow(w), inputGC, x1, y1, x2, y2);
	    }
	    else if (drawingMode == POINT) {
			// IDK
	    }
	    else if (drawingMode == RECTANGLE) {
			countRectangleParameters(x1, y1, x2, y2);
			XSetForeground(XtDisplay(w), inputGC, drawAreaPixel ^ fillColorFgPixel);
			XFillRectangle(XtDisplay(w), XtWindow(w), inputGC, rectX, rectY, abs(x1 - x2), abs(y1 - y2));
			XSetForeground(XtDisplay(w), inputGC, drawAreaPixel ^ lineColorFgPixel);
			XDrawRectangle(XtDisplay(w), XtWindow(w), inputGC, rectX, rectY, abs(x1 - x2), abs(y1 - y2));
		}
		
    }
}

/*
 * "draw" callback function
 */
void drawCB(Widget w, XtPointer client_data, XtPointer call_data) {
    Arg al[4];
    int ac;
    XGCValues v;
    XmDrawingAreaCallbackStruct *d = (XmDrawingAreaCallbackStruct*) call_data;

    switch (d->event->type) {
		case ButtonPress:
	 		if (d->event->xbutton.button == Button1) {
				button_pressed = 1;
				x1 = d->event->xbutton.x;
				y1 = d->event->xbutton.y;
	    	}
	    	break;

		case ButtonRelease:
	    	if (d->event->xbutton.button == Button1) {
				if (++nobjects > maxobjects) {
		    		maxobjects += OBJECTS_ALLOC_STEP;
		    		objects = (XSegment*) XtRealloc((char*)objects,
		      		(Cardinal)(sizeof(XSegment) * maxobjects));
				}

				objects[nobjects - 1].x1 = x1;
				objects[nobjects - 1].y1 = y1;
				objects[nobjects - 1].x2 = d->event->xbutton.x;
				objects[nobjects - 1].y2 = d->event->xbutton.y;

				button_pressed = 0;	

				if (!drawGC) {
				    ac = 0;
				    XtSetArg(al[ac], XmNforeground, &v.foreground); ac++;
				    XtGetValues(w, al, ac);
				    drawGC = XCreateGC(XtDisplay(w), XtWindow(w),
					GCForeground, &v);
				}
				// XDrawLine(XtDisplay(w), XtWindow(w), drawGC,
				  // x1, y1, d->event->xbutton.x, d->event->xbutton.y);
			}
	    	break;
    }
}

/*
 * "Expose" callback function
 */
/* ARGSUSED */
void ExposeCB(Widget w, XtPointer client_data, XtPointer call_data)
{

    if (nobjects <= 0)
	return;
    if (!drawGC)
	drawGC = XCreateGC(XtDisplay(w), XtWindow(w), 0, NULL);
    XDrawSegments(XtDisplay(w), XtWindow(w), drawGC, objects, nobjects);
}

/*
 * "Clear" button callback function
 */
/* ARGSUSED */
void ClearCB(Widget w, XtPointer client_data, XtPointer call_data)
{ 
    Widget wcd = (Widget) client_data;

    nobjects = 0;
    XClearWindow(XtDisplay(wcd), XtWindow(wcd));
}

/*
 * "Quit" button callback function
 */
/* ARGSUSED */
void QuitCB(Widget w, XtPointer client_data, XtPointer call_data)
{ 

    exit(0); 
}

void setDrawingMode (Widget w, XtPointer client_data, XtPointer call_data) {

	uintptr_t  selected = (uintptr_t ) client_data;
	if (selected == 0) {
		drawingMode = LINE;
	}
	else if (selected == 1) {
		drawingMode = POINT;
	}
	else if (selected == 2) {
		drawingMode = RECTANGLE;
	}
	else if (selected == 3) {
		drawingMode = ELLIPSE;
	}
	printf("Drawing Mode: %d\n", drawingMode);
}

void setFillMode (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void setLineWidth (Widget w, XtPointer client_data, XtPointer call_data) {

	uintptr_t  selected = (uintptr_t ) client_data;
	if (selected == 0) {
		lineWidth = 0;
	}
	else if (selected == 1) {
		lineWidth = 3;
	}
	else if (selected == 2) {
		lineWidth = 8;
	}
	printf("Line Width: %d\n", lineWidth);

}

void setLineType (Widget w, XtPointer client_data, XtPointer call_data) {

	uintptr_t  selected = (uintptr_t ) client_data;
	if (selected == 0) {
		lineType = SOLID;
	}
	else if (selected == 1) {
		lineType = DASHED;
	}
	printf("Line Type: %d\n", lineType);
}

void setLineColorFg (Widget w, XtPointer client_data, XtPointer call_data) {

    XColor xcolor, spare;	/* xlib color struct */

	uintptr_t  selected = (uintptr_t ) client_data;
	if (selected == 0) {
		if (XAllocNamedColor(display, cmap, "black", &xcolor, &spare) == 0) {
	        return;
		}
		lineColorFgPixel = xcolor.pixel;
	}
	else if (selected == 1) {
		if (XAllocNamedColor(display, cmap, "white", &xcolor, &spare) == 0) {
	        return;
		}
		lineColorFgPixel = xcolor.pixel;
	}
	else if (selected == 2) {
		if (XAllocNamedColor(display, cmap, "red", &xcolor, &spare) == 0) {
	        return;
		}
		lineColorFgPixel = xcolor.pixel;
	}
	else if (selected == 3) {
		if (XAllocNamedColor(display, cmap, "green", &xcolor, &spare) == 0) {
	        return;
		}
		lineColorFgPixel = xcolor.pixel;
	}
	printf("Line Fg Color: %d\n", lineColorFgPixel);
}

void setLineColorBg (Widget w, XtPointer client_data, XtPointer call_data) {
	XColor xcolor, spare;	/* xlib color struct */

	uintptr_t  selected = (uintptr_t ) client_data;
	if (selected == 0) {
		if (XAllocNamedColor(display, cmap, "black", &xcolor, &spare) == 0) {
	        return;
		}
		lineColorBgPixel = xcolor.pixel;
	}
	else if (selected == 1) {
		if (XAllocNamedColor(display, cmap, "white", &xcolor, &spare) == 0) {
	        return;
		}
		lineColorBgPixel = xcolor.pixel;
	}
	else if (selected == 2) {
		if (XAllocNamedColor(display, cmap, "red", &xcolor, &spare) == 0) {
	        return;
		}
		lineColorBgPixel = xcolor.pixel;
	}
	else if (selected == 3) {
		if (XAllocNamedColor(display, cmap, "green", &xcolor, &spare) == 0) {
	        return;
		}
		lineColorBgPixel = xcolor.pixel;
	}
	printf("Line Bg Color: %d\n", lineColorBgPixel);
}

void setFillColorFg (Widget w, XtPointer client_data, XtPointer call_data) {
	XColor xcolor, spare;	/* xlib color struct */

	uintptr_t  selected = (uintptr_t ) client_data;
	if (selected == 0) {
		if (XAllocNamedColor(display, cmap, "black", &xcolor, &spare) == 0) {
	        return;
		}
		fillColorFgPixel = xcolor.pixel;
	}
	else if (selected == 1) {
		if (XAllocNamedColor(display, cmap, "white", &xcolor, &spare) == 0) {
	        return;
		}
		fillColorFgPixel = xcolor.pixel;
	}
	else if (selected == 2) {
		if (XAllocNamedColor(display, cmap, "red", &xcolor, &spare) == 0) {
	        return;
		}
		fillColorFgPixel = xcolor.pixel;
	}
	else if (selected == 3) {
		if (XAllocNamedColor(display, cmap, "green", &xcolor, &spare) == 0) {
	        return;
		}
		fillColorFgPixel = xcolor.pixel;
	}
	printf("Fill Fg Color: %d\n", fillColorFgPixel);
}

void setFillColorBg (Widget w, XtPointer client_data, XtPointer call_data) {

	XColor xcolor, spare;	/* xlib color struct */

	uintptr_t  selected = (uintptr_t ) client_data;
	if (selected == 0) {
		if (XAllocNamedColor(display, cmap, "black", &xcolor, &spare) == 0) {
	        return;
		}
		fillColorBgPixel = xcolor.pixel;
	}
	else if (selected == 1) {
		if (XAllocNamedColor(display, cmap, "white", &xcolor, &spare) == 0) {
	        return;
		}
		fillColorBgPixel = xcolor.pixel;
	}
	else if (selected == 2) {
		if (XAllocNamedColor(display, cmap, "red", &xcolor, &spare) == 0) {
	        return;
		}
		fillColorBgPixel = xcolor.pixel;
	}
	else if (selected == 3) {
		if (XAllocNamedColor(display, cmap, "green", &xcolor, &spare) == 0) {
	        return;
		}
		fillColorBgPixel = xcolor.pixel;
	}
	printf("Fill Bg Color: %d\n", fillColorBgPixel);
}

int main(int argc, char **argv){
    XtAppContext app_context;
    Widget topLevel, mainWin, frame, drawArea, rowColumn, quitBtn, clearBtn,
    	shapesMenu, controlMenu, drawingModeMenu, fillMenu, lineWidthMenu,
    	lineColorFgMenu, lineColorBgMenu, fillColorFgMenu, fillColorBgMenu,
    	lineTypeMenu, colorMenu;

    /*
     * Register the default language procedure
     */
    XtSetLanguageProc(NULL, (XtLanguageProc)NULL, NULL);

    topLevel = XtVaAppInitialize(
      &app_context,		 	/* Application context */
      "Draw",				/* Application class */
      NULL, 0,				/* command line option list */
      &argc, argv,			/* command line args */
      NULL,				/* for missing app-defaults file */
      NULL);				/* terminate varargs list */

    mainWin = XtVaCreateManagedWidget(
      "mainWin",			/* widget name */
      xmMainWindowWidgetClass,		/* widget class */
      topLevel,				/* parent widget*/
      XmNcommandWindowLocation, XmCOMMAND_BELOW_WORKSPACE,
      NULL);				/* terminate varargs list */

    frame = XtVaCreateManagedWidget(
      "frame",				/* widget name */
      xmFrameWidgetClass,		/* widget class */
      mainWin,				/* parent widget */
      NULL);				/* terminate varargs list */

    drawArea = XtVaCreateManagedWidget(
      "drawingArea",			/* widget name */
      xmDrawingAreaWidgetClass,		/* widget class */
      frame,				/* parent widget*/
      XmNwidth, 200,			/* set startup width */
      XmNheight, 100,			/* set startup height */
      NULL);				/* terminate varargs list */

/*
    XSelectInput(XtDisplay(drawArea), XtWindow(drawArea), 
      KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask
      | Button1MotionMask );
*/
        
    rowColumn = XtVaCreateManagedWidget(
      "rowColumn",			/* widget name */
      xmRowColumnWidgetClass,		/* widget class */
      mainWin,				/* parent widget */
      XmNentryAlignment, XmALIGNMENT_CENTER,	/* alignment */
      XmNorientation, XmVERTICAL,	/* orientation */
      XmNpacking, XmPACK_COLUMN,	/* packing mode */
      NULL);				/* terminate varargs list */

	shapesMenu = XtVaCreateManagedWidget(
      "shapesMenu",			/* widget name */
      xmRowColumnWidgetClass,		/* widget class */
      rowColumn,				/* parent widget */
      XmNentryAlignment, XmALIGNMENT_CENTER,	/* alignment */
      XmNorientation, XmHORIZONTAL,	/* orientation */
      XmNpacking, XmPACK_COLUMN,	/* packing mode */
      NULL);	

	colorMenu = XtVaCreateManagedWidget(
      "colorMenu",			/* widget name */
      xmRowColumnWidgetClass,		/* widget class */
      rowColumn,				/* parent widget */
      XmNentryAlignment, XmALIGNMENT_CENTER,	/* alignment */
      XmNorientation, XmHORIZONTAL,	/* orientation */
      XmNpacking, XmPACK_COLUMN,	/* packing mode */
      NULL);	

	controlMenu = XtVaCreateManagedWidget(
      "controlMenu",			/* widget name */
      xmRowColumnWidgetClass,		/* widget class */
      rowColumn,				/* parent widget */
      XmNentryAlignment, XmALIGNMENT_CENTER,	/* alignment */
      XmNorientation, XmHORIZONTAL,	/* orientation */
      XmNpacking, XmPACK_COLUMN,	/* packing mode */
      NULL);	

	clearBtn = XtVaCreateManagedWidget(
      "Clear",				/* widget name */
      xmPushButtonWidgetClass,		/* widget class */
      controlMenu,			/* parent widget*/
      NULL);				/* terminate varargs list */

    quitBtn = XtVaCreateManagedWidget(
      "Quit",				/* widget name */
      xmPushButtonWidgetClass,		/* widget class */
      controlMenu,			/* parent widget*/
      NULL);				/* terminate varargs list */

    // drawing modes - point, line, rectangle, ellipse
    // will be inside rowColumn

    XmString drawingModeLabel = XmStringCreateLocalized("Mode");
    XmString drawingModeLine = XmStringCreateLocalized("Line");
    XmString drawingModePoint = XmStringCreateLocalized("Point");
    XmString drawingModeRectangle = XmStringCreateLocalized("Rectangle");
    XmString drawingModeEllipse = XmStringCreateLocalized("Ellipse");

    drawingModeMenu = XmVaCreateSimpleOptionMenu (
    	shapesMenu,
    	"drawingModeMenu",
    	drawingModeLabel, 
    	'x', 
    	0,
    	setDrawingMode,
        XmVaPUSHBUTTON, drawingModeLine, '0', NULL, NULL,
        XmVaPUSHBUTTON, drawingModePoint, '1', NULL, NULL,
        XmVaPUSHBUTTON, drawingModeRectangle, '2', NULL, NULL,
        XmVaPUSHBUTTON, drawingModeEllipse, '3', NULL, NULL,
        NULL
    );

    XmStringFree(drawingModeLabel);
    XmStringFree(drawingModeLine);
    XmStringFree(drawingModePoint);
    XmStringFree(drawingModeRectangle);
    XmStringFree(drawingModeEllipse);
    XtManageChild(drawingModeMenu);

    // rectangle or ellipse can be filled or not

    XmString fillingLabel = XmStringCreateLocalized("Filling");
    XmString shapeFilled = XmStringCreateLocalized("Filled");
    XmString shapeOutlined = XmStringCreateLocalized("Outlined");

    fillMenu = XmVaCreateSimpleOptionMenu (
    	shapesMenu,
    	"fillMenu",
    	fillingLabel, 
    	'x', 
    	0,
    	setFillMode,
        XmVaPUSHBUTTON, shapeFilled, '0', NULL, NULL,
        XmVaPUSHBUTTON, shapeOutlined, '1', NULL, NULL,
        NULL
    );

    XmStringFree(fillingLabel);
    XmStringFree(shapeFilled);
    XmStringFree(shapeOutlined);
    XtManageChild(fillMenu);

    // line width, can be 0, 3 ,8

    XmString lineWidthLabel = XmStringCreateLocalized("Line Width");
    XmString width0 = XmStringCreateLocalized("0");
    XmString width3 = XmStringCreateLocalized("3");
    XmString width8 = XmStringCreateLocalized("8");

    lineWidthMenu = XmVaCreateSimpleOptionMenu (
    	shapesMenu,
    	"lineWidthMenu",
    	lineWidthLabel, 
    	'x', 
    	0,
    	setLineWidth,
        XmVaPUSHBUTTON, width0, '0', NULL, NULL,
        XmVaPUSHBUTTON, width3, '1', NULL, NULL,
        XmVaPUSHBUTTON, width8, '2', NULL, NULL,
        NULL
    );

    XmStringFree(lineWidthLabel);
    XmStringFree(width0);
    XmStringFree(width3);
    XmStringFree(width8);
    XtManageChild(lineWidthMenu);

    // line type, can be solid or dashed

    XmString lineTypeLabel = XmStringCreateLocalized("Line Type");
    XmString lineSolid = XmStringCreateLocalized("solid");
    XmString lineDashed = XmStringCreateLocalized("dashed");

    lineTypeMenu = XmVaCreateSimpleOptionMenu (
    	shapesMenu,
    	"lineTypeMenu",
    	lineTypeLabel, 
    	'x', 
    	0,
    	setLineType,
        XmVaPUSHBUTTON, lineSolid, '0', NULL, NULL,
        XmVaPUSHBUTTON, lineDashed, '1', NULL, NULL,
        NULL
    );

    XmStringFree(lineTypeLabel);
    XmStringFree(lineSolid);
    XmStringFree(lineDashed);
    XtManageChild(lineTypeMenu);

    // line color, foreground, can be black, white, red, green

    XmString lineColorFgLabel = XmStringCreateLocalized("Line Color (Fg)");
    XmString lineColorFg1 = XmStringCreateLocalized("Black");
    XmString lineColorFg2 = XmStringCreateLocalized("White");
    XmString lineColorFg3 = XmStringCreateLocalized("Red");
    XmString lineColorFg4 = XmStringCreateLocalized("Green");

    lineColorFgMenu = XmVaCreateSimpleOptionMenu (
    	colorMenu,
    	"lineColorFgMenu",
    	lineColorFgLabel, 
    	'x', 
    	0,
    	setLineColorFg,
        XmVaPUSHBUTTON, lineColorFg1, '0', NULL, NULL,
        XmVaPUSHBUTTON, lineColorFg2, '1', NULL, NULL,
        XmVaPUSHBUTTON, lineColorFg3, '2', NULL, NULL,
        XmVaPUSHBUTTON, lineColorFg4, '3', NULL, NULL,
        NULL
    );

    XmStringFree(lineColorFgLabel);
    XmStringFree(lineColorFg1);
    XmStringFree(lineColorFg2);
    XmStringFree(lineColorFg3);
    XmStringFree(lineColorFg4);
    XtManageChild(lineColorFgMenu);

    // line color, background, can be black, white, red, green

    XmString lineColorBgLabel = XmStringCreateLocalized("Line Color (Bg)");
    XmString lineColorBg1 = XmStringCreateLocalized("Black");
    XmString lineColorBg2 = XmStringCreateLocalized("White");
    XmString lineColorBg3 = XmStringCreateLocalized("Red");
    XmString lineColorBg4 = XmStringCreateLocalized("Green");

    lineColorBgMenu = XmVaCreateSimpleOptionMenu (
    	colorMenu,
    	"lineColorBgMenu",
    	lineColorBgLabel, 
    	'x', 
    	0,
    	setLineColorBg,
        XmVaPUSHBUTTON, lineColorBg1, '1', NULL, NULL,
        XmVaPUSHBUTTON, lineColorBg2, '2', NULL, NULL,
        XmVaPUSHBUTTON, lineColorBg3, '3', NULL, NULL,
        XmVaPUSHBUTTON, lineColorBg4, '4', NULL, NULL,
        NULL
    );

    XmStringFree(lineColorBgLabel);
    XmStringFree(lineColorBg1);
    XmStringFree(lineColorBg2);
    XmStringFree(lineColorBg3);
    XmStringFree(lineColorBg4);
    XtManageChild(lineColorBgMenu);

    // fill color, foreground, can be black, white, red, green

    XmString fillColorFgLabel = XmStringCreateLocalized("Fill Color (Fg)");
    XmString fillColorFg1 = XmStringCreateLocalized("Black");
    XmString fillColorFg2 = XmStringCreateLocalized("White");
    XmString fillColorFg3 = XmStringCreateLocalized("Red");
    XmString fillColorFg4 = XmStringCreateLocalized("Green");

    fillColorFgMenu = XmVaCreateSimpleOptionMenu (
    	colorMenu,
    	"fillColorFgMenu",
    	fillColorFgLabel, 
    	'x', 
    	0,
    	setFillColorFg,
        XmVaPUSHBUTTON, fillColorFg1, '1', NULL, NULL,
        XmVaPUSHBUTTON, fillColorFg2, '2', NULL, NULL,
        XmVaPUSHBUTTON, fillColorFg3, '3', NULL, NULL,
        XmVaPUSHBUTTON, fillColorFg4, '4', NULL, NULL,
        NULL
    );

    XmStringFree(fillColorFgLabel);
    XmStringFree(fillColorFg1);
    XmStringFree(fillColorFg2);
    XmStringFree(fillColorFg3);
    XmStringFree(fillColorFg4);
    XtManageChild(fillColorFgMenu);

    // fill color, background, can be black, white, red, green

    XmString fillColorBgLabel = XmStringCreateLocalized("Fill Color (Bg)");
    XmString fillColorBg1 = XmStringCreateLocalized("Black");
    XmString fillColorBg2 = XmStringCreateLocalized("White");
    XmString fillColorBg3 = XmStringCreateLocalized("Red");
    XmString fillColorBg4 = XmStringCreateLocalized("Green");

    fillColorBgMenu = XmVaCreateSimpleOptionMenu (
    	colorMenu,
    	"fillColorBgMenu",
    	fillColorBgLabel, 
    	'x', 
    	0,
    	setFillColorBg,
        XmVaPUSHBUTTON, fillColorBg1, '1', NULL, NULL,
        XmVaPUSHBUTTON, fillColorBg2, '2', NULL, NULL,
        XmVaPUSHBUTTON, fillColorBg3, '3', NULL, NULL,
        XmVaPUSHBUTTON, fillColorBg4, '4', NULL, NULL,
        NULL
    );

    XmStringFree(fillColorBgLabel);
    XmStringFree(fillColorBg1);
    XmStringFree(fillColorBg2);
    XmStringFree(fillColorBg3);
    XmStringFree(fillColorBg4);
    XtManageChild(fillColorBgMenu);


    XmMainWindowSetAreas(mainWin, NULL, rowColumn, NULL, NULL, frame);

    XtAddCallback(drawArea, XmNinputCallback, drawCB, drawArea);
    XtAddEventHandler(drawArea, ButtonMotionMask, False, inputEH, NULL);
    XtAddCallback(drawArea, XmNexposeCallback, ExposeCB, drawArea);
	display = XtDisplay(drawArea);
	cmap = DefaultColormap(display, DefaultScreen(display));
	XtVaGetValues(drawArea, XmNbackground, &drawAreaPixel, NULL);


    XtAddCallback(clearBtn, XmNactivateCallback, ClearCB, drawArea);
    XtAddCallback(quitBtn, XmNactivateCallback, QuitCB, 0);


	// XtAddCallback(drawingModeMenu, XmNactivateCallback, setDrawingMode, drawingModeMenu);
	// XtAddCallback(fillMenu, XmNactivateCallback, setFillMode, fillMenu);
	// XtAddCallback(lineWidthMenu, XmNactivateCallback, setLineWidth, lineWidthMenu);
	// XtAddCallback(lineColorFgMenu, XmNactivateCallback, setLineColorFg, lineColorFgMenu);
	// XtAddCallback(lineColorBgMenu, XmNactivateCallback, setLineColorBg, lineColorBgMenu);
	// XtAddCallback(fillColorFgMenu, XmNactivateCallback, setFillColorFg, fillColorFgMenu);
	// XtAddCallback(fillColorBgMenu, XmNactivateCallback, setFillColorBg, fillColorBgMenu);
	// XtAddCallback(lineTypeMenu, XmNactivateCallback, setLineType, lineTypeMenu);


    XtRealizeWidget(topLevel);

    XtAppMainLoop(app_context);

    return 0;
}

