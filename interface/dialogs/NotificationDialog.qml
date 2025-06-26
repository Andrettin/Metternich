import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: notification_dialog
	panel: 5
	height: column.y + column.height + 8 * scale_factor
	
	property var portrait_object: null
	readonly property string portrait_identifier: portrait_object !== null ? (portrait_object.class_name === "metternich::character" ? portrait_object.game_data.portrait.identifier : portrait_object.identifier) : ""
	property string effects_text: ""
	property string text: ""
	property string second_button_text: ""
	property var second_button_effects: null
	
	Column {
		id: column
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		spacing: 16 * scale_factor
		
		PortraitButton {
			id: portrait
			anchors.horizontalCenter: parent.horizontalCenter
			portrait_identifier: notification_dialog.portrait_identifier
			visible: portrait_identifier.length > 0
			tooltip: is_character ? portrait_object.full_name : ""
			
			readonly property bool is_character: portrait_object !== null && portrait_object.class_name === "metternich::character"
		}
		
		SmallText {
			id: effects_text_label
			anchors.horizontalCenter: parent.horizontalCenter
			text: format_text(notification_dialog.effects_text)
			wrapMode: Text.WordWrap
			width: Math.min(effects_text_label_proxy.contentWidth, parent.width)
			visible: notification_dialog.effects_text.length > 0
			
			SmallText { //used to measure text, avoiding the binding loop of using the main text label's content width directly, given the wrap mode
				id: effects_text_label_proxy
				anchors.horizontalCenter: parent.horizontalCenter
				text: effects_text_label.text
				opacity: 0
			}
		}
		
		SmallText {
			id: text_label
			anchors.horizontalCenter: parent.horizontalCenter
			text: format_text(notification_dialog.text)
			wrapMode: Text.WordWrap
			width: Math.min(text_label_proxy.contentWidth, parent.width)
			visible: notification_dialog.text.length > 0
			
			SmallText { //used to measure text, avoiding the binding loop of using the main text label's content width directly, given the wrap mode
				id: text_label_proxy
				anchors.horizontalCenter: parent.horizontalCenter
				text: text_label.text
				opacity: 0
			}
		}
		
		Column {
			anchors.left: parent.left
			anchors.right: parent.right
			spacing: 8 * scale_factor
			
			TextButton {
				id: second_button
				anchors.horizontalCenter: parent.horizontalCenter
				text: second_button_text
				width: 128 * scale_factor
				visible: second_button_text.length > 0 && second_button_effects
				onClicked: {
					notification_dialog.close()
					notification_dialog.destroy()
					
					if (second_button_effects) {
						second_button_effects()
					}
				}
			}
			
			TextButton {
				id: ok_button
				anchors.horizontalCenter: parent.horizontalCenter
				text: "OK"
				width: 128 * scale_factor
				onClicked: {
					notification_dialog.close()
					notification_dialog.destroy()
				}
			}
		}
	}
}
