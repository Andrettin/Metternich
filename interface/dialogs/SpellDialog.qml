import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: spell_dialog
	title: "Spells"
	width: icon_button_width * spell_grid.columns + spell_grid.spacing * (spell_grid.columns - 1) + 8 * scale_factor * 2
	height: close_button.y + close_button.height + 8 * scale_factor
	
	enum Mode {
		All,
		Combat,
		Battle
	}

	property int mode: SpellDialog.Mode.All
	property var caster: null
	readonly property var spells: caster ? (mode === SpellDialog.Mode.Battle ? caster.game_data.battle_spells : caster.game_data.spells) : []
	readonly property int icon_button_width: 32 * scale_factor + 6 * scale_factor
	readonly property int icon_button_height: 32 * scale_factor + 6 * scale_factor
	
	MouseArea {
		anchors.fill: parent
		hoverEnabled: true
		
		onEntered: {
			metternich.set_current_cursor(metternich.defines.default_cursor)
		}
	}
	
	Flickable {
		id: spell_grid_view
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		height: icon_button_height * 4 + spell_grid.spacing * 3
		contentHeight: contentItem.childrenRect.height
		leftMargin: 0
		rightMargin: 0
		topMargin: 0
		bottomMargin: 0
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		
		Grid {
			id: spell_grid
			columns: 3
			spacing: 8 * scale_factor
			
			Repeater {
				model: spells
				
				IconButton {
					id: spell_icon
					icon_identifier: spell.icon.identifier
					tooltip: typeof status_text !== 'undefined' ? "" : format_text(small_text(spell.name + "\n\n" + costs_string))
					
					readonly property var spell: model.modelData
					readonly property string costs_string: "Mana Cost: " + spell.get_mana_cost()
					
					onClicked: {
						if (mode === SpellDialog.Mode.Combat || mode === SpellDialog.Mode.Battle) {
							combat.current_spell = spell
							spell_dialog.close()
						}
					}
					
					onHoveredChanged: {
						if (typeof status_text !== 'undefined') {
							if (hovered) {
								status_text = spell.name
								middle_status_text = costs_string
								if (mode === SpellDialog.Mode.Battle) {
									right_status_text = spell.get_battle_effects_string()
								}
							} else {
								status_text = ""
								middle_status_text = ""
								right_status_text = ""
							}
						}
					}
				}
			}
		}
	}
	
	TextButton {
		id: close_button
		anchors.top: spell_grid_view.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "Close"
		onClicked: {
			spell_dialog.close()
		}
	}
}
