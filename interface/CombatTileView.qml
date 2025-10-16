import QtQuick
import QtQuick.Controls

Item {
	id: tile
	implicitWidth: tile_size
	implicitHeight: tile_size
	
	readonly property bool tile_hovered: tile_mouse_area.containsMouse
	readonly property point tile_pos: Qt.point(column, row)
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
	
	/*
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
	*/
	
	Item {
		id: selection_rectangle_area
		anchors.fill: parent
		visible: tile_hovered
		
		Rectangle {
			anchors.top: parent.top
			anchors.left: parent.left
			color: "white"
			width: 1 * scale_factor
			height: 8 * scale_factor
		}
		
		Rectangle {
			anchors.bottom: parent.bottom
			anchors.left: parent.left
			color: "white"
			width: 1 * scale_factor
			height: 8 * scale_factor
		}
		
		Rectangle {
			anchors.top: parent.top
			anchors.right: parent.right
			color: "white"
			width: 1 * scale_factor
			height: 8 * scale_factor
		}
		
		Rectangle {
			anchors.bottom: parent.bottom
			anchors.right: parent.right
			color: "white"
			width: 1 * scale_factor
			height: 8 * scale_factor
		}
		
		Rectangle {
			anchors.top: parent.top
			anchors.left: parent.left
			color: "white"
			width: 1 * scale_factor
			height: 8 * scale_factor
		}
		
		Rectangle {
			anchors.top: parent.top
			anchors.left: parent.left
			color: "white"
			width: 8 * scale_factor
			height: 1 * scale_factor
		}
		
		Rectangle {
			anchors.bottom: parent.bottom
			anchors.left: parent.left
			color: "white"
			width: 8 * scale_factor
			height: 1 * scale_factor
		}
		
		Rectangle {
			anchors.top: parent.top
			anchors.right: parent.right
			color: "white"
			width: 8 * scale_factor
			height: 1 * scale_factor
		}
		
		Rectangle {
			anchors.bottom: parent.bottom
			anchors.right: parent.right
			color: "white"
			width: 8 * scale_factor
			height: 1 * scale_factor
		}
	}
	
	MouseArea {
		id: tile_mouse_area
		anchors.fill: parent
		hoverEnabled: true
		
		/*
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
		*/
		
		onEntered: {
			var text = "(" + column + ", " + row + ")"
			
			if (character !== null) {
				var type_name = character.monster_type ? character.monster_type.name : (character.game_data.character_class ? (character.game_data.character_class.name + " " + character.game_data.level) : character.species.name)
				text += " " + (character.full_name.length > 0 ? (character.full_name + " (" + type_name + ")") : type_name)
			}
			
			/*
			text += " ("
			
			var explored = metternich.game.player_country.game_data.is_tile_explored(tile_pos)
			
			if (!explored) {
				text += "Unexplored"
			} else if (site !== null && site.settlement && site.game_data.holding_type) {
				text += site.game_data.holding_type.name
				
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
			*/
			
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
}
