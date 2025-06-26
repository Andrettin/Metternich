import QtQuick
import QtQuick.Controls

Item {
	id: tile
	implicitWidth: tile_size
	implicitHeight: tile_size
	
	readonly property bool tile_selected: site !== null && selected_site === site && !selected_garrison
	readonly property bool civilian_unit_interactable: civilian_unit !== null && civilian_unit.owner === metternich.game.player_country
	readonly property point tile_pos: Qt.point(column, row)
	readonly property bool is_center_tile: province !== null && province.game_data.center_tile_pos === tile_pos
	readonly property var entering_army: (site !== null && site.game_data.visiting_armies.length > 0) ? site.game_data.visiting_armies[0] : ((province !== null && is_center_tile && province.game_data.entering_armies.length > 0) ? province.game_data.entering_armies[0] : null)
	readonly property bool show_failed_prospection: selected_civilian_unit !== null && selected_civilian_unit.type.prospector && prospected && resource === null
	property string saved_status_text: ""
	
	Repeater {
		model: base_image_sources
		
		TileImage {
			id: base_terrain_image
			tile_image_source: "image://" + modelData
			can_be_subtile: true
			x: (index === 1 || index === 3) ? 32 * scale_factor : 0
			y: index >= 2 ? 32 * scale_factor : 0
		}
	}
	
	Repeater {
		model: underlay_image_sources
		
		TileImage {
			id: underlay_image
			tile_image_source: "image://" + modelData
			can_be_subtile: true
			x: (index === 1 || index === 3) ? 32 * scale_factor : 0
			y: index >= 2 ? 32 * scale_factor : 0
		}
	}
	
	Repeater {
		model: image_sources
		
		TileImage {
			id: terrain_image
			tile_image_source: "image://" + modelData
			can_be_subtile: true
			x: (index === 1 || index === 3) ? 32 * scale_factor : 0
			y: index >= 2 ? 32 * scale_factor : 0
		}
	}
	
	Repeater {
		model: overlay_image_sources
		
		TileImage {
			id: overlay_image
			tile_image_source: "image://" + modelData
			can_be_subtile: true
			x: (index === 1 || index === 3) ? 32 * scale_factor : 0
			y: index >= 2 ? 32 * scale_factor : 0
		}
	}
	
	Repeater {
		model: object_image_sources
		
		TileImage {
			id: object_image
			tile_image_source: "image://" + modelData
		}
	}
	
	Image {
		id: garrison_icon
		anchors.left: parent.left
		anchors.leftMargin: visible && is_on_water ? (Math.floor(tile_size / 2) - Math.floor(width / 2)) : (8 * scale_factor)
		anchors.top: parent.top
		anchors.topMargin: visible && is_on_water ? (Math.floor(tile_size / 2) - Math.floor(height / 2)) : (8 * scale_factor)
		source: "image://icon/" + (visible && is_on_water ? (
			(province.game_data.military_unit_category_counts.length > 0 && province.game_data.get_country_military_unit_category_counts(metternich.game.player_country).length > 0) ?
				province.game_data.get_country_military_unit_icon(metternich.game.player_country).identifier
				: province.game_data.get_military_unit_icon().identifier
		) : "garrison") + (selected ? "/selected" : "")
		visible: province !== null && is_center_tile && province.game_data.military_unit_category_counts.length > 0
		
		readonly property bool is_on_water: province !== null && is_center_tile && province.water_zone
		readonly property bool selected: visible && selected_province === province && selected_garrison
	}
	
	Image {
		id: resource_icon
		anchors.left: garrison_icon.left
		anchors.leftMargin: 8 * scale_factor + 2 * scale_factor
		anchors.top: parent.top
		anchors.topMargin: 8 * scale_factor
		source: resource ? ("image://icon/" + (site && site.game_data.resource_improvement ? site.game_data.resource_improvement.icon.identifier : resource.tiny_icon.identifier)) : "image://empty/"
		visible: province !== null && is_center_tile && resource !== null && site && (site.game_data.settlement_type !== null || (improvement !== null && improvement.resource === null))
	}
	
	Image {
		id: depot_icon
		anchors.left: resource_icon.left
		anchors.leftMargin: 8 * scale_factor + 2 * scale_factor
		anchors.top: parent.top
		anchors.topMargin: 8 * scale_factor
		source: (site && site.game_data.depot_improvement !== null) ? ("image://icon/" + site.game_data.depot_improvement.icon.identifier) : "image://empty/"
		visible: site && site.game_data.depot_improvement !== null
	}
	
	Image {
		id: port_icon
		anchors.left: depot_icon.left
		anchors.leftMargin: 8 * scale_factor + 2 * scale_factor
		anchors.top: parent.top
		anchors.topMargin: 8 * scale_factor
		source: (site && site.game_data.port_improvement !== null) ? ("image://icon/" + site.game_data.port_improvement.icon.identifier) : "image://empty/"
		visible: site && site.game_data.port_improvement !== null
	}
	
	Rectangle {
		id: selection_rectangle
		anchors.fill: parent
		color: "transparent"
		border.color: "white"
		border.width: 1 * scale_factor
		visible: tile_selected
	}
	
	TileImage {
		id: failed_prospection_image
		tile_image_source: show_failed_prospection ? "image://icon/skull" : "image://empty/"
		visible: show_failed_prospection
	}
	
	TileImage {
		id: entering_army_image
		tile_image_source: entering_army !== null ? "image://icon/flag" : "image://empty/"
		visible: entering_army !== null && tile_detail_mode === false
	}
	
	TileImage {
		id: civilian_unit_image
		tile_image_source: civilian_unit ? (
			"image://icon/" + civilian_unit.icon.identifier + (unit_selected ? "/selected" : (civilian_unit.moving && !civilian_unit.working ? "/grayscale" : ""))
		) : "image://empty/"
		visible: civilian_unit !== null && tile_detail_mode === false
		
		readonly property bool unit_selected: selected_civilian_unit === civilian_unit
		
		Image {
			id: working_icon
			anchors.left: parent.left
			anchors.bottom: parent.bottom
			source: "image://icon/cog"
			fillMode: Image.Pad
			visible: civilian_unit !== null && civilian_unit.working
		}
	}
	
	TinyText {
		anchors.fill: parent
		text: upper_label
		visible: upper_label.length > 0
		wrapMode: Text.Wrap
		horizontalAlignment: Text.AlignHCenter
		verticalAlignment: Text.AlignTop
	}
	
	Item {
		id: tile_detail_item
		anchors.fill: parent
		visible: site && site.game_data.resource_improvement !== null && resource !== null && tile_detail_mode
		
		LargeText {
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.verticalCenter: parent.verticalCenter
			anchors.verticalCenterOffset: -24 * scale_factor
			text: resource && resource.commodity && site && site.game_data.commodity_outputs.length > 0 ? site.game_data.get_commodity_output(resource.commodity) : "" //refer to the commodity_outputs property so that this will be reevaluated if it changes (e.g. if the value changes)
			visible: tile_detail_resource_icon.visible
		}
		
		Image {
			id: tile_detail_resource_icon
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.verticalCenter: parent.verticalCenter
			source: resource && resource.commodity ? ("image://icon/" + resource.commodity.icon.identifier) : "image://empty/"
			fillMode: Image.Pad
			visible: site && site.game_data.resource_improvement !== null && resource !== null
		}
	}
	
	MouseArea {
		anchors.fill: parent
		hoverEnabled: true
		
		onReleased: {
			var explored = metternich.game.player_country.game_data.is_tile_explored(tile_pos)
			
			if (!explored) {
				selected_civilian_unit = null
				selected_site = null
				selected_province = null
				selected_garrison = false
				return
			}
			
			if (selected_civilian_unit !== null && !selected_civilian_unit.moving && !selected_civilian_unit.working) {
				if (tile_pos === selected_civilian_unit.tile_pos) {
					if (selected_civilian_unit.can_build_on_tile()) {
						selected_civilian_unit.build_on_tile()
						selected_civilian_unit = null
						selected_site = null
						selected_province = null
						selected_garrison = false
						return
					}
				} else if (civilian_unit === null && selected_civilian_unit.can_move_to(tile_pos)) {
					selected_civilian_unit.move_to(tile_pos)
					selected_civilian_unit = null
					selected_site = null
					selected_province = null
					selected_garrison = false
					return
				}
			}
			
			if (metternich.selected_military_units.length > 0) {
				metternich.move_selected_military_units_to(tile_pos)
				selected_civilian_unit = null
				selected_site = null
				selected_province = null
				selected_garrison = false
				return
			}
			
			if (civilian_unit !== null && civilian_unit_interactable && civilian_unit !== selected_civilian_unit && !civilian_unit.moving && !civilian_unit.working && (selected_site === null || site !== selected_site || selected_garrison)) {
				selected_civilian_unit = civilian_unit
				selected_site = null
				selected_province = null
			} else if (site !== null && (site !== selected_site || selected_garrison) && (site.settlement || resource || improvement)) {
				selected_site = site
				selected_province = province
				selected_civilian_unit = null
			} else {
				selected_civilian_unit = null
				selected_site = null
				selected_province = null
			}
			
			selected_garrison = false
		}
		
		onDoubleClicked: {
			//maybe move cancellations should be done by a cancel dialog when left-clicking the civilian unit instead
			if (civilian_unit !== null && civilian_unit_interactable) {
				if (civilian_unit.moving) {
					civilian_unit.cancel_move()
				} else if (civilian_unit.working) {
					civilian_unit.cancel_work()
				}
			}
		}
		
		onEntered: {
			var text = "(" + column + ", " + row + ") "
			
			text += "("
			
			var explored = metternich.game.player_country.game_data.is_tile_explored(tile_pos)
			
			if (!explored) {
				text += "Unexplored"
			} else if (site !== null && site.settlement && site.game_data.settlement_type) {
				text += site.game_data.settlement_type.name
				
				if (resource !== null) {
					text += ") ("
					if (site.game_data.resource_improvement !== null) {
						text += site.game_data.resource_improvement.name
						if (resource.natural_wonder) {
							text += ") (Natural Wonder"
						}
					} else if (resource.natural_wonder) {
						text += "Natural Wonder"
					} else {
						text += resource.name
					}
				}
			} else if (improvement !== null) {
				text += improvement.name
				if (resource !== null && resource.natural_wonder) {
					text += ") (Natural Wonder"
				}
			} else if (resource !== null) {
				if (resource.natural_wonder) {
					text += "Natural Wonder"
				} else {
					text += resource.name
				}
			} else {
				text += terrain.name
			}
			
			text += ") "
			
			if (river === true && !province.water_zone) {
				text += "(River) "
			}
			
			if (pathway !== null) {
				text += "(" + pathway.name + ") "
			}
			
			if (explored) {
				if (site !== null && (improvement !== null || resource !== null || site.settlement)) {
					text += site.game_data.current_cultural_name
				}
				
				if (province !== null) {
					if (site !== null && (improvement !== null || resource !== null || site.settlement)) {
						text += ", "
					}
					
					text += province.game_data.current_cultural_name
					
					if (province.game_data.owner !== null) {
						text += ", " + province.game_data.owner.name
					}
				}
			}
			
			if (site !== null && (site.settlement || resource !== null)) {
				text += " (Max Transport: " + site.game_data.transport_level + ")"
			}
			
			status_text = text
			saved_status_text = text
		}
		onExited: {
			//only clear the status text on exit if it was actually still the text set by this
			if (status_text === saved_status_text) {
				status_text = ""
				saved_status_text = ""
			}
		}
	}
	
	MouseArea {
		anchors.horizontalCenter: garrison_icon.horizontalCenter
		anchors.verticalCenter: garrison_icon.verticalCenter
		width: Math.min(garrison_icon.width + 8 * scale_factor, tile_size)
		height: Math.min(garrison_icon.height + 8 * scale_factor, tile_size)
		hoverEnabled: true
		enabled: garrison_icon.visible && province !== null && ((province.water_zone && province.game_data.military_unit_category_counts.length > 0 && province.game_data.get_country_military_unit_category_counts(metternich.game.player_country).length > 0) || (province.game_data.owner !== null && (province.game_data.owner === metternich.game.player_country || province.game_data.owner.game_data.is_any_vassal_of(metternich.game.player_country))))
		visible: enabled
		
		onReleased: {
			selected_civilian_unit = null
			
			if (garrison_icon.selected) {
				selected_site = null
				selected_province = null
				selected_garrison = false
			} else {
				selected_site = site
				selected_province = province
				selected_garrison = true
			}
		}
		
		onEntered: {
			status_text = "Garrison"
		}
	}
	
	MouseArea {
		anchors.horizontalCenter: resource_icon.horizontalCenter
		anchors.verticalCenter: resource_icon.verticalCenter
		width: Math.min(resource_icon.width + 8 * scale_factor, tile_size)
		height: Math.min(resource_icon.height + 8 * scale_factor, tile_size)
		hoverEnabled: true
		enabled: resource_icon.visible
		visible: enabled
		
		onEntered: {
			if (site.game_data.resource_improvement !== null) {
				status_text = site.game_data.resource_improvement.name
			} else {
				status_text = resource.name
			}
		}
	}
	
	MouseArea {
		anchors.horizontalCenter: depot_icon.horizontalCenter
		anchors.verticalCenter: depot_icon.verticalCenter
		width: Math.min(depot_icon.width + 8 * scale_factor, tile_size)
		height: Math.min(depot_icon.height + 8 * scale_factor, tile_size)
		hoverEnabled: true
		enabled: depot_icon.visible
		visible: enabled
		
		onEntered: {
			status_text = site.game_data.depot_improvement.name
		}
	}
	
	MouseArea {
		anchors.horizontalCenter: port_icon.horizontalCenter
		anchors.verticalCenter: port_icon.verticalCenter
		width: Math.min(port_icon.width + 8 * scale_factor, tile_size)
		height: Math.min(port_icon.height + 8 * scale_factor, tile_size)
		hoverEnabled: true
		enabled: port_icon.visible
		visible: enabled
		
		onEntered: {
			status_text = site.game_data.port_improvement.name
		}
	}
}
