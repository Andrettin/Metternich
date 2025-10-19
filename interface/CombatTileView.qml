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
		id: tile_movable_area
		anchors.fill: parent
		visible: movable_to
		
		Rectangle {
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.bottom: tile_movable_rectangle.bottom
			color: "black"
			width: 4 * scale_factor
			height: 1 * scale_factor
		}
		
		Rectangle {
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.top: tile_movable_rectangle.bottom
			color: "black"
			width: 2 * scale_factor
			height: 1 * scale_factor
		}
		
		Rectangle {
			id: tile_movable_rectangle
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.verticalCenter: parent.verticalCenter
			color: retreatable_at ? "white" : Qt.rgba(95.0 / 255.0, 186.0 / 255.0, 75.0 / 255.0, 1)
			width: 2 * scale_factor
			height: 2 * scale_factor
		}
	}
	
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
		
		onReleased: {
			metternich.game.current_combat.set_target(tile_pos)
		}
		
		onEntered: {
			var text = "(" + column + ", " + row + ")"
			
			if (character !== null) {
				var type_name = character.monster_type ? character.monster_type.name : (character.game_data.character_class ? (character.game_data.character_class.name + " " + character.game_data.level) : character.species.name)
				text += " " + (character.full_name.length > 0 ? (character.full_name + " (" + type_name + ")") : type_name)
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
}
