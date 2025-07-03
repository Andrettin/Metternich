import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import technology_model

TreeView {
	id: tech_tree
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	model: TechnologyModel {}
	delegate: TreeViewDelegate {
		implicitWidth: tech_tree.width
		font.family: berenika_font.name
		font.pixelSize: 10 * scale_factor
		Material.theme: Material.Dark
		Material.accent: "olive"
		
		onClicked: {
			technology_dialog.technology = technology
			technology_dialog.open()
		}
	}
	
	Component.onCompleted: {
		tech_tree.expandRecursively()
	}
}
