import QtQuick
import QtQuick.Controls
import "./dialogs"

Item {
	id: domestic_view
	
	enum Mode {
		Court,
		Government,
		Religion,
		Production,
		BalanceBook,
		DomainHistory
	}
	
	property var domain: null
	readonly property var domain_game_data: domain ? domain.game_data : null
	readonly property var domain_turn_data: domain ? domain.turn_data : null
	readonly property var ruler: domain_game_data ? domain_game_data.government.ruler : null
	property string status_text: ""
	property string middle_status_text: ""
	property string right_status_text: ""
	property bool show_buttons: true
	
	TiledBackground {
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		interface_style: "dark_wood_boards"
		frame_count: 8
	}
	
	CourtView {
		id: court_view
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		visible: domestic_view_mode === DomesticView.Mode.Court
	}
	
	GovernmentView {
		id: government_view
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		visible: domestic_view_mode === DomesticView.Mode.Government
	}
	
	ReligionView {
		id: religion_view
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		visible: domestic_view_mode === DomesticView.Mode.Religion
	}
	
	ProductionView {
		id: production_view
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		visible: domestic_view_mode === DomesticView.Mode.Production
	}
	
	BalanceBookView {
		id: balance_book_view
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		visible: domestic_view_mode === DomesticView.Mode.BalanceBook
	}
	
	DomainHistoryView {
		id: domain_history_view
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		visible: domestic_view_mode === DomesticView.Mode.DomainHistory
	}
	
	DomesticButtonPanel {
		id: button_panel
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: parent.right
		show_buttons: domestic_view.show_buttons
	}
	
	CourtInfoPanel {
		id: infopanel
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.left: parent.left
	}
	
	StatusBar {
		id: status_bar
		anchors.bottom: parent.bottom
		anchors.left: infopanel.right
		anchors.right: button_panel.left
	}
	
	TopBar {
		id: top_bar
		anchors.top: parent.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
	}
	
	CharacterDialog {
		id: character_dialog
	}
	
	InventoryDialog {
		id: inventory_dialog
	}
	
	SpellDialog {
		id: spell_dialog
		mode: SpellDialog.Mode.All
	}
	
	ItemShopDialog {
		id: item_shop_dialog
	}
	
	SellItemsDialog {
		id: sell_items_dialog
	}
	
	RecipeDialog {
		id: recipe_dialog
	}
}
