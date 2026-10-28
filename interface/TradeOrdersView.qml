import QtQuick
import QtQuick.Controls

ListView {
	id: trade_orders_view
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	model: domain_game_data.economy.tradeable_commodities
	delegate: Item {
		width: trade_orders_view.width
		height: commodity_icon.height + 4 * scale_factor * 2
		
		readonly property var commodity: model.modelData
		
		Image {
			id: commodity_icon
			anchors.verticalCenter: parent.verticalCenter
			anchors.left: parent.left
			anchors.leftMargin: 4 * scale_factor
			source: "image://icon/" + commodity.icon.identifier
			fillMode: Image.Pad
		}
		
		SmallText {
			id: commodity_label
			text: commodity.name
			anchors.verticalCenter: parent.verticalCenter
			anchors.left: commodity_icon.right
			anchors.leftMargin: 8 * scale_factor
		}
		
		Rectangle {
			id: commodity_border
			color: "gray"
			anchors.bottom: parent.bottom
			anchors.left: parent.left
			anchors.right: parent.right
			height: 1 * scale_factor
			visible: index !== (trade_orders_view.model.length - 1)
		}
		
		TextButton {
			id: bid_button
			anchors.verticalCenter: parent.verticalCenter
			anchors.right: offer_button.left
			anchors.rightMargin: 16 * scale_factor
			text: qsTr("Bid")
			width: 48 * scale_factor
			height: 24 * scale_factor
			highlighted: domain_game_data.economy.bids.length > 0 && domain_game_data.economy.get_bid_int(commodity) > 0
			
			onClicked: {
				if (domain_game_data.economy.get_bid_int(commodity) === 0) {
					domain_game_data.economy.set_bid(commodity, 10)
				} else {
					domain_game_data.economy.set_bid(commodity, 0)
				}
			}
		}
		
		TextButton {
			id: offer_button
			anchors.verticalCenter: parent.verticalCenter
			anchors.right: price_area.left
			anchors.rightMargin: 16 * scale_factor
			text: qsTr("Offer")
			width: 48 * scale_factor
			height: 24 * scale_factor
			highlighted: domain_game_data.economy.offers.length > 0 && domain_game_data.economy.get_offer_int(commodity) > 0
			visible: domain_game_data.economy.stored_commodities.length > 0 && domain_game_data.economy.get_stored_commodity_int(commodity)
			
			onClicked: {
				if (domain_game_data.economy.get_offer_int(commodity) === 0) {
					domain_game_data.economy.set_offer(commodity, domain_game_data.economy.get_stored_commodity_int(commodity))
				} else {
					domain_game_data.economy.set_offer(commodity, 0)
				}
			}
		}
		
		Item {
			id: price_area
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.right: available_area.left
			width: 96 * scale_factor
			
			Rectangle {
				id: price_border
				color: "gray"
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				anchors.left: parent.left
				width: 1 * scale_factor
			}
			
			SmallText {
				id: price_label
				text: metternich.game.get_price_qstring(commodity)
				anchors.verticalCenter: parent.verticalCenter
				anchors.right: parent.right
				anchors.rightMargin: 32 * scale_factor
			}
		}
		
		Item {
			id: available_area
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.right: quantity_to_trade_area.left
			width: 96 * scale_factor
			
			Rectangle {
				id: available_border
				color: "gray"
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				anchors.left: parent.left
				width: 1 * scale_factor
			}
			
			SmallText {
				id: available_label
				text: number_string(domain_game_data.economy.get_stored_commodity_int(commodity))
				anchors.verticalCenter: parent.verticalCenter
				anchors.right: parent.right
				anchors.rightMargin: 32 * scale_factor
			}
		}
		
		Item {
			id: quantity_to_trade_area
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.right: parent.right
			width: 256 * scale_factor
			
			Rectangle {
				id: quantity_to_trade_border
				color: "gray"
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				anchors.left: parent.left
				width: 1 * scale_factor
			}
			
			CustomSlider {
				id: quantity_to_trade_slider
				anchors.verticalCenter: parent.verticalCenter
				anchors.left: parent.left
				anchors.leftMargin: 16 * scale_factor
				anchors.right: parent.right
				anchors.rightMargin: 16 * scale_factor
				value: domain_game_data.economy.bids.length > 0 && is_bid ? domain_game_data.economy.get_bid_int(commodity) : (domain_game_data.economy.offers.length > 0 && is_offer ? domain_game_data.economy.get_offer_int(commodity) : 0)
				max_value: is_bid ? 10 : Math.max(10, domain_game_data.economy.stored_commodities.length > 0 ? domain_game_data.economy.get_stored_commodity_int(commodity) : 0)
				visible: is_bid || is_offer
				
				readonly property bool is_bid: domain_game_data.economy.bids.length > 0 && domain_game_data.economy.get_bid_int(commodity) > 0
				readonly property bool is_offer: domain_game_data.economy.offers.length > 0 && domain_game_data.economy.get_offer_int(commodity) > 0
				
				onDecremented: {
					if (is_bid) {
						if (domain_game_data.economy.get_bid_int(commodity) == 1) {
							return
						}
						
						domain_game_data.economy.change_bid(commodity, -1)
					} else if (is_offer) {
						if (domain_game_data.economy.get_offer_int(commodity) == 1) {
							return
						}
						
						domain_game_data.economy.change_offer(commodity, -1)
					}
				}
				
				onIncremented: {
					if (is_bid) {
						domain_game_data.economy.change_bid(commodity, 1)
					} else if (is_offer) {
						domain_game_data.economy.change_offer(commodity, 1)
					}
				}
				
				onClicked: function(target_value) {
					if (target_value == 0) {
						target_value = 1
					}
					
					if (is_bid) {
						domain_game_data.economy.set_bid(commodity, target_value)
					} else if (is_offer) {
						domain_game_data.economy.set_offer(commodity, target_value)
					}
				}
			}
		}
	}
}
