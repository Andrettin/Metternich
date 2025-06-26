import QtQuick
import QtQuick.Controls
import QtCharts

PopulationChart {
	id: chart
	
	Connections {
		target: chart.data_source
		ignoreUnknownSignals: true //as there may be no selected data source
		
		function onIdeology_counts_changed() {
			chart.update_chart()
		}
	}

	function update_chart() {
		pie_series.clear()

		if (chart.population_data === null && chart.data_source === null) {
			return
		}
		
		var population_per_ideology = chart.population_data ? chart.population_data : chart.data_source.ideology_counts
		for (var i = 0; i < population_per_ideology.length; i++) {
			var ideology = population_per_ideology[i].key
			var count = population_per_ideology[i].value
			var pie_slice = pie_series.append(ideology.name, count)
			pie_slice.color = ideology.color
			pie_slice.borderColor = "black"
		}
	}
}
