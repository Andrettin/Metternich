import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import family_tree_model 1.0

TreeView {
	id: family_tree_view
	height: Math.min(256 * scale_factor, contentHeight)
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	
	property var character: null
	
	model: FamilyTreeModel {
		character: family_tree_view.character
	}
	delegate: TreeViewDelegate {
		implicitWidth: family_tree_view.width
		font.family: berenika_font.name
		font.pixelSize: 10 * scale_factor
		Material.theme: Material.Dark
		
		onClicked: {
		}
		
		onHoveredChanged: {
			var text = ""
			
			if (model.character) {
				text = model.character.full_name
			}
			
			if (typeof status_text !== 'undefined') {
				if (hovered) {
					status_text = text
				} else {
					if (status_text === text) {
						status_text = ""
					}
				}
			}
		}
	}
	
	function expand_to_character() {
		var character_index = model.get_character_model_index(character)
		family_tree_view.expandToIndex(character_index)
		family_tree_view.forceLayout()
		family_tree_view.positionViewAtRow(rowAtIndex(character_index), Qt.AlignVCenter)
	}
}
