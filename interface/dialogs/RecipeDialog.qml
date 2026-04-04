import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: recipe_dialog
	title: "Recipes"
	width: icon_button_width * recipe_grid.columns + recipe_grid.spacing * (recipe_grid.columns - 1) + 8 * scale_factor * 2
	height: close_button.y + close_button.height + 8 * scale_factor
	
	property var crafter: null
	readonly property var recipes: crafter ? crafter.game_data.recipes : []
	readonly property int icon_button_width: 32 * scale_factor + 6 * scale_factor
	readonly property int icon_button_height: 32 * scale_factor + 6 * scale_factor
	
	MouseArea {
		anchors.fill: parent
		hoverEnabled: true
		
		onEntered: {
			metternich.set_current_cursor(metternich.defines.default_cursor)
		}
	}
	
	NormalText {
		id: craft_label
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: crafter ? ("Craft: " + crafter.game_data.craft + "/" + crafter.game_data.max_craft) : ""
	}
	
	Flickable {
		id: recipe_grid_view
		anchors.top: craft_label.visible ? craft_label.bottom : title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		height: Math.min(icon_button_height * 4 + recipe_grid.spacing * 3, contentHeight)
		contentHeight: contentItem.childrenRect.height
		leftMargin: 0
		rightMargin: 0
		topMargin: 0
		bottomMargin: 0
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		
		Grid {
			id: recipe_grid
			columns: 3
			spacing: 8 * scale_factor
			
			Repeater {
				model: recipes
				
				IconButton {
					id: recipe_icon
					icon_identifier: recipe.icon.identifier
					tooltip: typeof status_text !== 'undefined' ? "" : format_text(small_text(recipe.name + "\n\n" + costs_string))
					
					readonly property var recipe: model.modelData
					readonly property string formula_string: recipe.formula_string
					
					onClicked: {
						if (crafter.game_data.can_craft_recipe(recipe)) {
							crafter.game_data.craft_recipe(recipe)
						} else {
							metternich.defines.error_sound.play()
						}
					}
					
					onHoveredChanged: {
						if (typeof status_text !== 'undefined') {
							if (hovered) {
								status_text = recipe.name
								middle_status_text = formula_string
							} else {
								status_text = ""
								middle_status_text = ""
							}
						}
					}
				}
			}
		}
	}
	
	TextButton {
		id: close_button
		anchors.top: recipe_grid_view.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "Close"
		onClicked: {
			recipe_dialog.close()
		}
	}
}
