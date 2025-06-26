import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

Flickable {
	id: balance_book_view
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	
	Column {
		anchors.top: parent.top
		anchors.left: parent.left
		anchors.right: parent.right
		
		Item {
			id: transactions_area
			anchors.left: parent.left
			anchors.right: parent.right
			height: Math.max(income_transactions_column.height, expense_transactions_column.height)
			
			Column {
				id: income_transactions_column
				anchors.top: parent.top
				anchors.left: parent.left
				anchors.right: parent.horizontalCenter
				
				Repeater {
					model: country_turn_data.income_transactions
					
					Item {
						width: income_transactions_column.width
						height: transaction_icon.height + 4 * scale_factor * 2
						
						readonly property var income_transaction: model.modelData
						
						Image {
							id: transaction_icon
							anchors.verticalCenter: parent.verticalCenter
							anchors.left: parent.left
							anchors.leftMargin: 4 * scale_factor
							source: "image://icon/" + income_transaction.icon.identifier
							fillMode: Image.Pad
						}
						
						SmallText {
							id: commodity_label
							text: income_transaction.name
							anchors.verticalCenter: parent.verticalCenter
							anchors.left: transaction_icon.right
							anchors.leftMargin: 8 * scale_factor
						}
						
						Item {
							id: description_area
							anchors.top: parent.top
							anchors.bottom: parent.bottom
							anchors.right: parent.right
							width: 224 * scale_factor
							
							SmallText {
								id: description_label
								text: income_transaction.description
								anchors.verticalCenter: parent.verticalCenter
								anchors.left: parent.left
								anchors.leftMargin: 8 * scale_factor
								anchors.right: parent.right
								anchors.rightMargin: 8 * scale_factor
								wrapMode: Text.WordWrap
							}
						}
					}
				}
			}
			
			Rectangle {
				id: income_expenses_border
				color: "gray"
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				anchors.horizontalCenter: parent.horizontalCenter
				width: 1 * scale_factor
			}
			
			Column {
				id: expense_transactions_column
				anchors.top: parent.top
				anchors.left: parent.horizontalCenter
				anchors.right: parent.right
				
				Repeater {
					model: country_turn_data.expense_transactions
					
					Item {
						width: expense_transactions_column.width
						height: transaction_icon.height + 4 * scale_factor * 2
						
						readonly property var expense_transaction: model.modelData
						
						Image {
							id: transaction_icon
							anchors.verticalCenter: parent.verticalCenter
							anchors.left: parent.left
							anchors.leftMargin: 4 * scale_factor
							source: "image://icon/" + expense_transaction.icon.identifier
							fillMode: Image.Pad
						}
						
						SmallText {
							id: commodity_label
							text: expense_transaction.name
							anchors.verticalCenter: parent.verticalCenter
							anchors.left: transaction_icon.right
							anchors.leftMargin: 8 * scale_factor
						}
						
						Item {
							id: description_area
							anchors.top: parent.top
							anchors.bottom: parent.bottom
							anchors.right: parent.right
							width: 224 * scale_factor
							
							SmallText {
								id: description_label
								text: expense_transaction.description
								anchors.verticalCenter: parent.verticalCenter
								anchors.left: parent.left
								anchors.leftMargin: 8 * scale_factor
								anchors.right: parent.right
								anchors.rightMargin: 8 * scale_factor
								wrapMode: Text.WordWrap
							}
						}
					}
				}
			}
		}
		
		Rectangle {
			id: totals_border
			color: "gray"
			anchors.left: parent.left
			anchors.right: parent.right
			height: 1 * scale_factor
			visible: transactions_area.height > 0
		}
		
		Item {
			anchors.horizontalCenter: parent.horizontalCenter
			width: 192 * scale_factor
			height: balance_value_label.y + balance_value_label.height + 4 * scale_factor
			
			SmallText {
				id: total_income_label
				text: "Total Income:"
				anchors.top: parent.top
				anchors.topMargin: 4 * scale_factor
				anchors.left: parent.left
				anchors.leftMargin: 4 * scale_factor
			}
			
			SmallText {
				id: total_income_value_label
				text: "$" + number_string(country_turn_data.total_income)
				anchors.top: total_income_label.top
				anchors.right: parent.right
				anchors.rightMargin: 4 * scale_factor
			}
			
			SmallText {
				id: total_expense_label
				text: "Total Expense:"
				anchors.top: total_income_label.bottom
				anchors.topMargin: 4 * scale_factor
				anchors.left: total_income_label.left
			}
			
			SmallText {
				id: total_expense_value_label
				text: "-$" + number_string(country_turn_data.total_expense)
				anchors.top: total_expense_label.top
				anchors.right: total_income_value_label.right
			}
			
			SmallText {
				id: total_inflation_change_label
				text: "Total Inflation Change:"
				anchors.top: total_expense_label.bottom
				anchors.topMargin: 4 * scale_factor
				anchors.left: total_income_label.left
			}
			
			SmallText {
				id: total_inflation_change_value_label
				text: "+" + country_turn_data.total_inflation_change + "%"
				anchors.top: total_inflation_change_label.top
				anchors.right: total_income_value_label.right
			}
			
			Rectangle {
				id: balance_border
				color: "gray"
				anchors.top: total_inflation_change_label.bottom
				anchors.topMargin: 4 * scale_factor
				anchors.left: parent.left
				anchors.right: parent.right
				height: 1 * scale_factor
			}
			
			SmallText {
				id: balance_label
				text: "Balance:"
				anchors.top: balance_border.bottom
				anchors.topMargin: 4 * scale_factor
				anchors.left: total_income_label.left
			}
			
			SmallText {
				id: balance_value_label
				text: (country_turn_data.total_expense > country_turn_data.total_income ? "-" : "") + "$" + number_string(Math.abs(country_turn_data.total_income - country_turn_data.total_expense))
				anchors.top: balance_label.top
				anchors.right: total_income_value_label.right
			}
		}
	}
}
