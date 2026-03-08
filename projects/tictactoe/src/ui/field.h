//------------------------------------------------------------------
// field.h
//
// Author:           JuanJakobo
// Date:             19.4.2020
// Description:      Handles game fields
//-------------------------------------------------------------------

#ifndef FIELD
#define FIELD

#include "string"

using namespace std;

class Field {
    public:
        Field(int startX,int startY,int width,int height);

        void setContent(string con);
        string getContent();
        bool operator==(const Field& f);
        void drawField();
        void updateFieldArea();
        bool pointInsideField(int x, int y);
        bool containsContent();

    private:
        int startX;
        int startY;
        int width;
        int height;
        string content;
};

#endif
