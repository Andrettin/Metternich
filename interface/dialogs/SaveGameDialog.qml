import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Dialogs
import ".."

FileDialog {
	id: save_game_dialog
	title: qsTr("Save Game")
	defaultSuffix: ".met"
	currentFolder: "file:" + metternich.save_path
	nameFilters: ["Metternich Save Files (*.met)"]
	fileMode: FileDialog.SaveFile
	
	onAccepted: {
		var file_url = save_game_dialog.selectedFile
		metternich.game.save(file_url)
		menu_dialog.close()
	}
}
