import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Dialogs
import ".."

FileDialog {
	id: load_game_dialog
	title: qsTr("Load Game")
	defaultSuffix: ".met"
	currentFolder: "file:" + metternich.save_path
	nameFilters: ["Metternich Save Files (*.met)"]
	fileMode: FileDialog.OpenFile
	
	onAccepted: {
		var file_url = save_game_dialog.selectedFile
		metternich.game.load(file_url)
	}
}
