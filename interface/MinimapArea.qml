import QtQuick
import QtQuick.Controls

Rectangle {
	id: minimap_area
	color: interface_background_color
	width: infopanel.width
	height: 128 * scale_factor + 2 * scale_factor
	clip: true
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "gray"
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: parent.right
		width: 1 * scale_factor
	}
	
	MouseArea {
		id: minimap_area_mouse_area
		anchors.fill: parent
		hoverEnabled: true
		
		onContainsMouseChanged: {
			if (containsMouse) {
				status_text = ""
			}
		}
	}
	
	Rectangle {
		id: minimap_borders
		anchors.top: minimap.top
		anchors.topMargin: -1 * scale_factor
		anchors.bottom: minimap.bottom
		anchors.bottomMargin: -1 * scale_factor
		anchors.left: minimap.left
		anchors.leftMargin: -1 * scale_factor
		anchors.right: minimap.right
		anchors.rightMargin: -1 * scale_factor
		color: "gray"
	}
	
	Minimap {
		id: minimap
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.verticalCenter: parent.verticalCenter
	}
	
	Column {
		id: minimap_mode_left_column
		anchors.right: minimap_borders.left
		anchors.rightMargin: 4 * scale_factor
		anchors.verticalCenter: parent.verticalCenter
		spacing: 4 * scale_factor
		
		IconButton {
			id: political_map_mode_button
			icon_identifier: "flag"
			highlighted: province_map.mode === ProvinceMap.Mode.Political
			
			onClicked: {
				province_map.mode = ProvinceMap.Mode.Political
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Political Map Mode"
				} else {
					status_text = ""
				}
			}
		}
		
		IconButton {
			id: site_map_mode_button
			icon_identifier: "settlement"
			highlighted: province_map.show_sites
			
			onClicked: {
				province_map.show_sites = !province_map.show_sites
				
				if (province_map.show_sites) {
					status_text = "Hide Sites"
				} else {
					status_text = "Show Sites"
				}
			}
			
			onHoveredChanged: {
				if (hovered) {
					if (province_map.show_sites) {
						status_text = "Hide Sites"
					} else {
						status_text = "Show Sites"
					}
				} else {
					status_text = ""
				}
			}
		}
	}
	
	Grid {
		id: minimap_mode_right_grid
		anchors.left: minimap_borders.right
		anchors.leftMargin: 4 * scale_factor
		anchors.verticalCenter: parent.verticalCenter
		spacing: 4 * scale_factor
		columns: 2
		
		TinyIconButton {
			id: trade_zone_map_mode_button
			icon_identifier: "trade_consulate"
			highlighted: province_map.mode === ProvinceMap.Mode.TradeZone
			
			onClicked: {
				province_map.mode = ProvinceMap.Mode.TradeZone
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Trade Zone Map Mode"
				} else {
					status_text = ""
				}
			}
		}
		
		TinyIconButton {
			id: temple_map_mode_button
			icon_identifier: "temple"
			highlighted: province_map.mode === ProvinceMap.Mode.Temple
			
			onClicked: {
				province_map.mode = ProvinceMap.Mode.Temple
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Temple Map Mode"
				} else {
					status_text = ""
				}
			}
		}
		
		TinyIconButton {
			id: terrain_map_mode_button
			icon_identifier: "alliance"
			highlighted: province_map.mode === ProvinceMap.Mode.Terrain
			
			onClicked: {
				province_map.mode = ProvinceMap.Mode.Terrain
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Terrain Map Mode"
				} else {
					status_text = ""
				}
			}
		}
		
		TinyIconButton {
			id: cultural_map_mode_button
			icon_identifier: "embassy"
			highlighted: province_map.mode === ProvinceMap.Mode.Cultural
			
			onClicked: {
				province_map.mode = ProvinceMap.Mode.Cultural
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Cultural Map Mode"
				} else {
					status_text = ""
				}
			}
		}
		
		TinyIconButton {
			id: religious_map_mode_button
			icon_identifier: "peace"
			highlighted: province_map.mode === ProvinceMap.Mode.Religious
			
			onClicked: {
				province_map.mode = ProvinceMap.Mode.Religious
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Religious Map Mode"
				} else {
					status_text = ""
				}
			}
		}
		
		TinyIconButton {
			id: technology_map_mode_button
			icon_identifier: "law"
			highlighted: province_map.mode === ProvinceMap.Mode.Technology
			
			onClicked: {
				province_map.mode = ProvinceMap.Mode.Technology
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Technology Map Mode"
				} else {
					status_text = ""
				}
			}
		}
	}
}
