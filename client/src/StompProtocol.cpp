#include "../include/StompProtocol.h"

StompProtocol::StompProtocol():isLogin(false)
{
}

StompProtocol::~StompProtocol()
{
    connectionHandler = nullptr;
}

void StompProtocol::process(Frame &input)
{
    // רותם, כאן ממלאים את כל הלוגיקה של הפרוטוקול
    // כלומר, כאן נבדוק את סוג הפריים ונטפל בהתאם
    // לדוגמא, אם הפריים הוא קונקט, ניצור קונקשיון הנדלר עם הארגומנטים המתאימים וכן הלאה לשאר הפריימים האפשריים
}