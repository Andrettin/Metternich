import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: construction_choice_dialog
	title: "Choose Construction"
	width: 256 * scale_factor
	height: content_height
	
	readonly property int content_height: construction_button_column.y + construction_button_column.height + 8 * scale_factor
	
	property var buildable_locations: []
	
	SmallText {
		id: text_label
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		text: "Which construction project shall we build next?"
		wrapMode: Text.WordWrap
	}
	
	Column {
		id: construction_button_column
		anchors.top: text_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		
		Repeater {
			model: buildable_locations
			
			TextButton {
				id: construction_button
				text: format_text(building.name + " (" + buildable_location.holding.game_data.current_cultural_name + ")")
				width: construction_choice_dialog.width - 16 * scale_factor
				tooltip: effects_string.length > 0 ? format_text(small_text(effects_string)) : small_text("No effect")
				
				readonly property var buildable_location: model.modelData
				readonly property var building: buildable_location.get_buildable_building()
				readonly property string effects_string: building.get_effects_string(buildable_location.holding, false)
				
				onClicked: {
					construction_choice_dialog.close()
					buildable_location.build_building(building)
				}
			}
		}
	}
}
