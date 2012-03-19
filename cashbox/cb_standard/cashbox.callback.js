
function refreshTable(uid, page, sort, order) {
	if (!uid || uid == 0) return;
	if(!page) page = 0;
	if(!sort) sort = 'desc';
	if(!order) order = 'Zeitstempel';

	jQuery.ajax({
		cache: false,
		url: Drupal.settings.basePath + '?q=cashbox/callback/' + uid,
		data: {uid: uid, page: page, sort: sort, order: order},
		dataType: 'text',
		error: function(request, status, error) {
			alert(status);
		},
		success: function(data, status, request) {
			var html = data;

			jQuery('#table-container').html(html);
			
			jQuery('#table-container th a').
				add('#table-container .pager-item a')
				.add('#table-container .pager-first a')
				.add('#table-container .pager-previous a')
				.add('#table-container .pager-next a')
				.add('#table-container .pager-last a')
					.click(function(el, a, b, c, d) {
						var url = jQuery.url(el.currentTarget.getAttribute('href'));
						refreshTable(url.param('uid'), url.param('page'), url.param('sort'), url.param('order'));
					
						return (false);
					});
		}
	});
}
	
function initializeTable(uid) {
	jQuery(document).ready(function() {
		refreshTable(uid);
	});
}
