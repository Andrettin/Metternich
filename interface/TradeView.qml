import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

Item {
	id: trade_view
	
	enum Mode {
		TradeOrders,
		BalanceBook
	}
	
	readonly property var country: metternich.game.player_country
	readonly property var country_game_data: country ? country.game_data : null
	readonly property var country_turn_data: country ? country.turn_data : null
	property string status_text: ""
	property string middle_status_text: ""
	
	TiledBackground {
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		interface_style: "dark_wood_boards"
		frame_count: 8
	}
	
	TradeOrdersView {
		id: trade_orders_view
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		visible: trade_view_mode === TradeView.Mode.TradeOrders
	}
	
	BalanceBookView {
		id: balance_book_view
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		visible: trade_view_mode === TradeView.Mode.BalanceBook
	}
	
	TradeButtonPanel {
		id: button_panel
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: parent.right
	}
	
	TradeInfoPanel {
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
		
		Item {
			id: price_top_area
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.right: available_top_area.left
			width: 96 * scale_factor
			visible: trade_view_mode === TradeView.Mode.TradeOrders
			
			SmallText {
				id: price_top_label
				text: "Price:"
				anchors.top: parent.top
				anchors.topMargin: 1 * scale_factor
				anchors.horizontalCenter: parent.horizontalCenter
			}
		}
		
		Item {
			id: available_top_area
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.right: quantity_to_trade_top_area.left
			width: 96 * scale_factor
			visible: trade_view_mode === TradeView.Mode.TradeOrders
			
			SmallText {
				id: available_top_label
				text: "Available:"
				anchors.top: parent.top
				anchors.topMargin: 1 * scale_factor
				anchors.horizontalCenter: parent.horizontalCenter
			}
		}
		
		Item {
			id: quantity_to_trade_top_area
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.right: parent.right
			width: 256 * scale_factor
			visible: trade_view_mode === TradeView.Mode.TradeOrders
			
			SmallText {
				id: quantity_to_trade_top_label
				text: "Quantity to Trade:"
				anchors.top: parent.top
				anchors.topMargin: 1 * scale_factor
				anchors.horizontalCenter: parent.horizontalCenter
			}
		}
	}
}
