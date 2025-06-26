import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: dialog
	title: "Appoint " + office_name
	width: 256 * scale_factor
	height: content_height
	
	readonly property int content_height: cancel_button.y + cancel_button.height + 8 * scale_factor
	
	property var office: null
	readonly property string office_name: office ? metternich.game.player_country.game_data.get_office_title_name_qstring(office) : ""
	readonly property var office_holders: office ? metternich.game.player_country.game_data.get_appointable_office_holders_qvariant_list(office) : []
	
	SmallText {
		id: text_label
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		text: "Whom shall we appoint " + office_name.toLowerCase() + "?"
		wrapMode: Text.WordWrap
	}
	
	Column {
		id: office_holder_button_column
		anchors.top: text_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		visible: office_holders.length > 0
		
		Repeater {
			model: office_holders
			
			TextButton {
				id: office_holder_button
				text: format_text(office_holder.full_name)
				width: dialog.width - 16 * scale_factor
				tooltip: tooltip_string.length > 0 ? format_text(small_text(tooltip_string)) : ""
				
				readonly property var office_holder: model.modelData
				readonly property string costs_string: costs_to_string(metternich.game.player_country.game_data.get_advisor_commodity_costs_qvariant_list(office))
				readonly property string modifier_string: office_holder.game_data.get_office_modifier_qstring(metternich.game.player_country, office)
				readonly property string tooltip_string: costs_string + (costs_string.length > 0 && modifier_string.length > 0 ? "\n\n" : "") + modifier_string
				
				onClicked: {
					if (metternich.game.player_country.game_data.can_appoint_office_holder(office, office_holder)) {
						metternich.game.player_country.game_data.set_appointed_office_holder(office, office_holder)
						dialog.close()
					} else {
						add_notification("Costs", metternich.game.player_country.game_data.interior_minister_portrait, "Your Excellency, we unfortunately cannot pay the costs of appointing a new " + office_name.toLowerCase() + ".", dialog.parent)
					}
				}
			}
		}
	}
	
	TextButton {
		id: cancel_button
		anchors.top: office_holder_button_column.visible ? office_holder_button_column.bottom : text_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "Cancel"
		onClicked: {
			dialog.close()
		}
	}
}
