
function refreshTable(uid, page, sort, order) {
	if (!uid || uid == 0) return;
	if(!page) page = 0;
	if(!sort) sort = 'desc';
	if(!order) order = 'Zeitstempel';

	jQuery.ajax({
		cache: false,
		url: Drupal.settings.basePath + '?q=cashbox/callback/books/' + uid,
		data: {uid: uid, page: page, sort: sort, order: order},
		dataType: 'text',
		error: function(request, status, error) {
			alert(status);
		},
		success: function(data, status, request) {
			var html = data;

			jQuery('#books_display-div').html(html);
			
			jQuery('#books_display-div th a').
				add('#books_display-div .pager-item a')
				.add('#books_display-div .pager-first a')
				.add('#books_display-div .pager-previous a')
				.add('#books_display-div .pager-next a')
				.add('#books_display-div .pager-last a')
					.click(function(el, a, b, c, d) {
						var url = jQuery.url(el.currentTarget.getAttribute('href'));
						refreshTable(url.param('uid'), url.param('page'), url.param('sort'), url.param('order'));
					
						return (false);
					});
		}
	});
}
	
function initializeTableMaster(uid) {
	jQuery(document).ready(function() {
		refreshTable(uid);
	});
}
