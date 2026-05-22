import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: cancel_construction_dialog
	width: 256 * scale_factor
	height: no_button.y + no_button.height + 8 * scale_factor
	title: "Cancel Pathway Construction?"
	
	property var province: null
	readonly property var under_construction_pathway: province ? province.game_data.under_construction_pathway : null
	
	SmallText {
		id: text
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		text: under_construction_pathway ? ("Do you wish to cancel the construction of a " + under_construction_pathway.name + " here?") : ""
		wrapMode: Text.WordWrap
		horizontalAlignment: Text.AlignHCenter
	}
	
	TextButton {
		id: yes_button
		anchors.top: text.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "Yes"
		onClicked: {
			province.game_data.cancel_pathway_construction()
			province = null
			cancel_construction_dialog.close()
		}
	}
	
	TextButton {
		id: no_button
		anchors.top: yes_button.bottom
		anchors.topMargin: 8 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "No"
		onClicked: {
			province = null
			cancel_construction_dialog.close()
		}
	}
}
