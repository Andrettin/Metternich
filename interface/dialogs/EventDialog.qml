import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: event_dialog
	panel: 5
	width: Math.max(content_width, default_width)
	height: Math.max(content_height, default_height)
	title: event_instance ? event_instance.name : ""
	
	readonly property int max_button_width: calculate_max_button_width(option_column) + 8 * scale_factor * 2
	readonly property int content_width: Math.max(max_button_width, title_item.contentWidth + 8 * scale_factor * 2)
	readonly property int content_height: description.y + description.contentHeight + 16 * scale_factor + option_column.height + 8 * scale_factor
	
	property var event_instance: null
	readonly property var option_names: event_instance ? event_instance.option_names : []
	readonly property var option_tooltips: event_instance ? event_instance.option_tooltips : []
	
	property bool option_picked: false
	
	PortraitButton {
		id: portrait
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		portrait_identifier: (event_instance && event_instance.event && event_instance.event.portrait) ? event_instance.event.portrait.identifier : ""
		visible: portrait_identifier.length > 0
		circle: event_instance && event_instance.event && event_instance.event.circular_portrait
	}
	
	SmallText {
		id: description
		anchors.top: portrait.visible ? portrait.bottom : title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		anchors.bottom: option_column.top
		anchors.bottomMargin: 16 * scale_factor
		text: event_instance ? format_text(event_instance.description) : ""
		wrapMode: Text.WordWrap
	}
	
	Column {
		id: option_column
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 8 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		
		Repeater {
			model: option_names
			
			TextButton {
				id: option_button
				text: format_text(model.modelData)
				width: event_dialog.width - 16 * scale_factor
				tooltip: format_text(small_text(option_tooltips[index]))
				
				onClicked: {
					if (option_picked) {
						//an option was already picked
						return
					}
					
					option_picked = true
					event_instance.choose_option(index)
				}
			}
		}
	}
	
	Connections {
		target: metternich
		
		function onEvent_closed(event_instance) {
			if (event_instance === event_dialog.event_instance) {
				event_dialog.close()
				event_dialog.destroy()
			}
		}
	}
}
