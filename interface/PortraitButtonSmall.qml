import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

IconButtonBase {
	id: button
	source: "image://portrait/" + portrait_identifier + "/small"
	use_margins: false
	
	property string portrait_identifier: ""
}
