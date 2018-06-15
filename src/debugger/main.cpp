#include "gbd_mainwindow.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	gbd::MainWindow w;
	w.show();
	return a.exec();
}
