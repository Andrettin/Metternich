#include <QApplication>

#include <stdexcept>

int main(int argc, char **argv)
{
	try {
		/*
		QApplication app(argc, argv);
		app.setApplicationName("Metternich");
		app.setApplicationVersion("1.0.0");
		app.setOrganizationName("Metternich");
		app.setOrganizationDomain("andrettin.github.io");
		*/

		return 0;
	} catch (const std::exception &exception) {
		//exception::report(exception);
		return -1;
	}
}
