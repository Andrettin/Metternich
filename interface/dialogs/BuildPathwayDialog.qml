import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: build_pathway_dialog
	width: 256 * scale_factor
	height: no_button.y + no_button.height + 8 * scale_factor
	title: pathway ? ("Build " + pathway.name + "?") : ""
	
	property var province: null
	property var pathway: null
	
	SmallText {
		id: text
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		text: pathway && province ? format_text("Do you wish to build a " + pathway.name + " here?\n\n" + pathway.get_commodity_costs_string_for_province(province)) : ""
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
			province.game_data.build_pathway(pathway)
			province = null
			pathway = null
			build_pathway_dialog.close()
		}
	}
	
	TextButton {
		id: no_button
		anchors.top: yes_button.bottom
		anchors.topMargin: 8 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "No"
		onClicked: {
			province.game_data.cancel_pathway_construction()
			province = null
			pathway = null
			build_pathway_dialog.close()
		}
	}
}
