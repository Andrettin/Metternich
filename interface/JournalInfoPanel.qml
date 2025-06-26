import QtQuick
import QtQuick.Controls

Rectangle {
	id: infopanel
	color: interface_background_color
	width: 64 * scale_factor
	clip: true
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "gray"
		anchors.top: parent.top
		anchors.topMargin: 15 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 15 * scale_factor
		anchors.left: parent.left
		width: 1 * scale_factor
	}
	
	IconButton {
		id: finished_mode_button
		anchors.bottom: active_mode_button.top
		anchors.bottomMargin: 4 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: "skull"
		highlighted: journal_view.mode === JournalView.Mode.Finished
		
		onClicked: {
			journal_view.mode = JournalView.Mode.Finished
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Finished Journal Entries"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: active_mode_button
		anchors.bottom: inactive_mode_button.top
		anchors.bottomMargin: 4 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: "newspaper"
		highlighted: journal_view.mode === JournalView.Mode.Active
		
		onClicked: {
			journal_view.mode = JournalView.Mode.Active
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Active Journal Entries"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: inactive_mode_button
		anchors.bottom: back_button.top
		anchors.bottomMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: "flag"
		highlighted: journal_view.mode === JournalView.Mode.Inactive
		
		onClicked: {
			journal_view.mode = JournalView.Mode.Inactive
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Inactive Journal Entries"
			} else {
				status_text = ""
			}
		}
	}
	
	TextButton {
		id: back_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 16 * scale_factor
		text: qsTr("Back")
		width: 48 * scale_factor
		height: 24 * scale_factor
		
		onClicked: {
			menu_stack.pop()
		}
	}
}
