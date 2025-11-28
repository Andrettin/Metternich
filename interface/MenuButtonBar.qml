import QtQuick
import QtQuick.Controls

Rectangle {
	id: menu_button_bar
	color: interface_background_color
	width: infopanel.width
	height: 50 * scale_factor
	clip: true
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "gray"
		anchors.top: parent.top
		anchors.topMargin: 15 * scale_factor
		anchors.bottom: parent.bottom
		anchors.right: parent.right
		width: 1 * scale_factor
	}
	
	Row {
		id: button_row
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 6 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.horizontalCenterOffset: Math.floor(-1 * scale_factor / 2)
		spacing: 4 * scale_factor
		
		IconButton {
			id: politics_button
			//icon_identifier: "flag"
			icon_identifier: "rifle_infantry_light_small"
			
			onReleased: {
				menu_stack.push("PoliticsView.qml", {
					country: metternich.game.player_country
				})
			}
			
			onHoveredChanged: {
				if (hovered) {
					//status_text = "View Politics"
					status_text = "View Court"
				} else {
					status_text = ""
				}
			}
		}
		
		IconButton {
			id: technology_button
			icon_identifier: "research"
			
			onReleased: {
				menu_stack.push("TechnologyView.qml")
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "View Technologies"
				} else {
					status_text = ""
				}
			}
		}
		
		IconButton {
			id: journal_button
			icon_identifier: "newspaper"
			
			onReleased: {
				menu_stack.push("JournalView.qml")
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "View Journal"
				} else {
					status_text = ""
				}
			}
		}
		
		/*
		IconButton {
			id: trade_button
			icon_identifier: "wealth"
			
			onReleased: {
				menu_stack.push("TradeView.qml")
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "View Trade"
				} else {
					status_text = ""
				}
			}
		}
		*/
		
		IconButton {
			id: diplomatic_map_button
			icon_identifier: "globe"
			
			onReleased: {
				menu_stack.push("DiplomaticView.qml", {
					start_tile_x: map_area_start_x + map_area_tile_width / 2,
					start_tile_y: map_area_start_y + map_area_tile_height / 2
				})
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "View Diplomatic Map"
				} else {
					status_text = ""
				}
			}
		}

		IconButton {
			id: menu_button
			icon_identifier: "cog"
			
			onReleased: {
				menu_dialog.open()
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "View Menu"
				} else {
					status_text = ""
				}
			}
		}
	}
}
