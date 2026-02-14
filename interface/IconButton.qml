import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

IconButtonBase {
	id: button
	source: "image://icon/" + icon_identifier
	use_margins: true
	
	property string icon_identifier: ""
}
