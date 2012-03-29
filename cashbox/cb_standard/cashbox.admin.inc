<?php
/**
 * Form function, called by drupal_get_form()
 * in cashbox_menu()
 */
function cashbox_admin($form, &$form_state) {
	$form['general'] = array(
		'#type'		=> 'fieldset',
		'#title'	=> t('General settings'),
	);
	$form['general']['cashbox_symbol'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Currency Symbol'),
		'#size'		=> 3,
		'#maxlength'	=> 5,
		'#default_value' => variable_get('cashbox_symbol', '$'),
		'#description'	=> t('Choose a currency symbol - real or fake, your choice'),
		'#required'	=> true,
	);
	$form['general']['cashbox_thousand'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Thousands separator'),
		'#description'	=> t('Please enter the correct thousands separator for your country - NOTE: all future inputs will be parsed according to this'),
		'#size'		=> 1,
		'#maxlength'	=> 1,
		'#default_value' => variable_get('cashbox_thousand', ','),
		'#field_prefix'	=> t('e.g. ') . variable_get('cashbox_symbol', '$') . ' 123',
		'#field_suffix'	=> '456' . variable_get('cashbox_decimal', '.') . '78',
	);
	$form['general']['cashbox_decimal'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Decimal separator'),
		'#description'	=> t('Please enter the correct decimal separator for your country - NOTE: all future inputs will be parsed according to this'),
		'#size'		=> 1,
		'#maxlength'	=> 1,
		'#default_value' => variable_get('cashbox_decimal', '.'),
		'#field_prefix'	=> t('e.g. ') . variable_get('cashbox_symbol', '$') . ' 123' . variable_get('cashbox_thousand', ',') . '456',
		'#field_suffix'	=> '78',
		'#required'	=> true,
	);
	$form['general']['cashbox_perPage'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Number of entries per page'),
		'#description'	=> t('Displayed balances per page, 0 for all on one page'),
		'#size'		=>  3,
		'#default_value' => variable_get('cashbox_perPage', 25),
	);
	if (module_exists('mobile_codes')) 
		$form['general']['cashbox_qr'] = array(
			'#type'		=> 'checkbox',
			'#title'	=> t('Display checksums as QR-Codes'),
			'#default_value' => variable_get('cashbox_qr', 1),
		);
	$form['general']['cashbox_taxonomy_default'] = array(
		'#type'		=> 'select',
		'#title'	=> t('Default Transaction-Type'),
		'#options'	=> _cashbox_get_transaction_terms(),
		'#default_value' => variable_get('cashbox_taxonomy_default'),
		'#description' 	=> t('Select the default transaction-type. You can change the vocabulary in the !tax section', array('!tax' => l(t('Taxonomy'), 'admin/structure/taxonomy/cb_transactions'))),
	);	
	$form['account'] = array(
		'#type' 	=> 'fieldset',
		'#title' 	=> t('Account Settings'),
		'#description' 	=> t('Define limits for transactions and accounts'),
		);
	$form['account']['cashbox_hardlimit'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Maximum overdraw'),
		'#default_value' => variable_get('cashbox_hardlimit', 50),
		'#field_prefix'	=> variable_get('cashbox_symbol') . '&nbsp;&#x2212;',
		'#field_suffix' => variable_get('cashbox_decimal', '.') . '00',
		'#size'		=> 5,
		'#maxlength'	=> 4,
		'#description'	=> t('Hard-limit for overdrawing any account (recommended)'),
		'#required'	=> true,
	);
	$form['account']['cashbox_transaction_limit'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Default transaction limit'),
		'#default_value' => variable_get('cashbox_transaction_limit', 50),
		'#field_prefix'	=> variable_get('cashbox_symbol') . '&nbsp;&#x00B1;',
		'#field_suffix'	=> variable_get('cashbox_decimal', '.') . '00',
		'#size'		=> 5,
		'#maxlength'	=> $form['account']['cashbox_hardlimit']['#maxlength'],
		'#description'	=> t('Maximum amount that can be transferred in one transaction (overruled by "Maximum overdraw" / special privliges may be applied)'),
	);

	return system_settings_form($form);
}


/**
  * Implements validation from the Form API.
  * 
  * @param $form
  *   A structured array containing the elements and properties of the form.
  * @param $form_state
  *   An array that stores information about the form's current state 
  *   during processing.
  */
function cashbox_admin_validate($form, &$form_state) {
	$form_state['values']['cashbox_hardlimit'] = abs(intval($form_state['values']['cashbox_hardlimit']));
	$form_state['values']['cashbox_transaction_limit'] = abs(intval($form_state['values']['cashbox_transaction_limit']));
	if ($form_state['values']['cashbox_thousand'] == $form_state['values']['cashbox_decimal'])
		form_set_error('cashbox_thousand', t('Thousands separator and decimal separator must not be the same'));
	if (is_numeric($form_state['values']['cashbox_decimal']))
		form_set_error('cashbox_decimal', t('Decimal separator must not be numeric'));
	if (is_numeric($form_state['values']['cashbox_thousand']))
		form_set_error('cashbox_thousand', t('Thousands separator must not be numeric'));
	if (!is_int((int)$form_state['values']['cashbox_hardlimit']))
		form_set_error('cashbox_hardlimit', t('Maximum overdraw has to be numeric'));
	if (!is_int((int)$form_state['values']['cashbox_transaction_limit']))
		form_set_error('cashbox_hardlimit', t('Transaction limit has to be numeric'));
	if (!is_numeric($form_state['values']['cashbox_perPage']) || $form_state['values']['cashbox_perPage'] < 0)
		form_set_error('cashbox_perPage', t('Invalid value for the maximum number of balances displayed per page'));
}
	

