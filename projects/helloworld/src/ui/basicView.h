#ifndef BASICVIEW
#define BASICVIEW

#include "inkview.h"

class BasicView {
public:
    BasicView(const irect *contentRect);
    ~BasicView();

    void clearContent();
    void clicked(int x, int y);

private:
    const irect *_contentRect;
    int _fontHeight = 35;
    ifont *_font = OpenFont("LiberationMono", _fontHeight, 1);
};

#endif
