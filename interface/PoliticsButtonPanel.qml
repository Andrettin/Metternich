import QtQuick
import QtQuick.Controls

Rectangle {
	id: button_panel
	color: interface_background_color
	width: 64 * scale_factor
	clip: true
	
	property bool show_buttons: true
	
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
	
	Column {
		id: button_column
		anchors.top: parent.top
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 4 * scale_factor
		visible: button_panel.show_buttons
		
		IconButton {
			id: court_button
			icon_identifier: "rifle_infantry_light_small"
			highlighted: politics_view_mode === PoliticsView.Mode.Court
			
			onClicked: {
				politics_view_mode = PoliticsView.Mode.Court
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "View Court"
				} else {
					status_text = ""
				}
			}
		}
		
		IconButton {
			id: government_button
			icon_identifier: "flag"
			highlighted: politics_view_mode === PoliticsView.Mode.Government
			visible: false
			
			onClicked: {
				politics_view_mode = PoliticsView.Mode.Government
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "View Government"
				} else {
					status_text = ""
				}
			}
		}
		
		IconButton {
			id: religion_button
			icon_identifier: "wooden_cross"
			highlighted: politics_view_mode === PoliticsView.Mode.Religion
			visible: false
			
			onClicked: {
				politics_view_mode = PoliticsView.Mode.Religion
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "View Religion"
				} else {
					status_text = ""
				}
			}
		}
		
		IconButton {
			id: domain_history_button
			icon_identifier: "painting"
			highlighted: politics_view_mode === PoliticsView.Mode.DomainHistory
			
			onClicked: {
				politics_view_mode = PoliticsView.Mode.DomainHistory
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "View Domain History"
				} else {
					status_text = ""
				}
			}
		}
	}
}
