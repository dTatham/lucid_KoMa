<?php

/**
 * Adjust accounts, called by drupal_get_form()
 * in cashbox_menu()
 */
function cashbox_adjust($form, &$form_state) {
	global $user;
	
	if (!isset($form_state['storage']['confirm'])) {
		$form['user'] = array(
			'#type'		=> 'textfield',
			'#title'	=> t('User'),
			'#size'		=> 20,
			'#maxlength'	=> 128,
			'#autocomplete_path' => 'cashbox/callback/transfer_autocomplete',
			'#required'	=> true,
		);
		$form['amount'] = array(
			'#title' => t('Amount'),
			'#type'	=> 'textfield',
			'#field_prefix' => variable_get('cashbox_symbol', '$'),
			'#size'	=> 5,
			'#maxlength' => 7,
			'#default_value' => '0' . variable_get('cashbox_decimal', '.') . '00',
			'#required'	=> true,
		);
		$form['message'] = array(
			'#title' => t('Comment'),
			'#type' => 'textfield',
			'#maxlength' => 255,
		);
		$form['submit'] = array(
			'#value' => t('Save'),
			'#type' => 'submit',
		);
		return $form;
	}
	else {
		$form['user'] = array(
			'#type'		=> 'hidden',
			'#title'	=> t('User'),
			'#size'		=> 20,
			'#maxlength'	=> 128,
			'#required'	=> true,
		);
		$form['amount'] = array(
			'#title' => t('Amount'),
			'#type'	=> 'hidden',
			'#field_prefix' => variable_get('cashbox_symbol', '$'),
			'#size'	=> 5,
			'#maxlength' => 7,
			'#default_value' => '0' . variable_get('cashbox_decimal', '.') . '00',
			'#required'	=> true,
		);
		$form['message'] = array(
			'#title' => t('Comment'),
			'#type' => 'hidden',
			'#maxlength' => 255,
		);

		drupal_add_css(drupal_get_path('module', 'cashbox') . '/cashbox.css');
		drupal_add_css(drupal_get_path('module', 'cashbox') . '/cashbox_print.css', array('media' => 'print'));

		$row = array(array(
			$form_state['values']['user'],
			$form_state['values']['message'],
			array('data' => '<div class="cashbox_symbol">' . variable_get('cashbox_symbol', '$') . '</div><div class="cashbox_amount">' . number_format($form_state['values']['amount'], 2, variable_get('cashbox_decimal', '.'), variable_get('cashbox_thousand')) . '</div>', 'class' => 'cashbox_amount ' . ($form_state['values']['amount'] < 0 ? 'cashbox_debit' : 'cashbox_credit')),
			array('data' => '<div class="cashbox_symbol">' . variable_get('cashbox_symbol', '$') . '</div><div class="cashbox_amount">' . number_format($form_state['values']['balance'], 2, variable_get('cashbox_decimal', '.'), variable_get('cashbox_thousand')) . '</div>', 'class' => 'cashbox_amount ' . ($form_state['values']['balance'] < 0 ? 'cashbox_debit' : 'cashbox_credit')),
		));
		$html = theme('table',  array(
			'header'        => array(t('User'), t('Comment'), t('Amount'), t('!user new balance', array('!user' => ($user->name == (isset($form_state['values']['user']) ? $form_state['values']['user'] : $user->name) ? 'Your' : $form_state['values']['user'].'\'s')))),
			'empty'         => t('No transactions available'),
			'rows'          => $row,
			'sticky'        => true,
			'caption'       => t('Please double-check your data'),
		)

	);

		return confirm_form($form, 
			t('Are you sure you want to proceed?'),
			isset($_GET['destination']) ? $_GET['destination'] : "cashbox/transfer/adjust", $html);
	}

}

function cashbox_adjust_validate($form, &$form_state) {
	global $user;
	$amount = str_replace(variable_get('cashbox_decimal', '.'), '.', str_replace(variable_get('cashbox_thousand', ','), '', $form_state['values']['amount']));
	$aff_user = _cashbox_fetch_data($form_state['values']['user']);

	if (empty($aff_user)) {
		form_set_error('user', t('User invalid.'));
		return;
	}
	if (!is_numeric($amount))
		form_set_error('amount', t('Amount invalid.'));
	else {
		form_set_value(array('#parents' => array('amount')), $amount, $form_state);
		form_set_value(array('#parents' => array('balance')), _cashbox_get_current_balance($aff_user->name)+$amount, $form_state);
	}

	form_set_value(array('#parents' => array('message')), filter_xss($form_state['values']['message'], array('em', 'strong', 'cite', 'blockquote', 'code', 'ul', 'ol', 'li', 'dl', 'dt', 'dd')), $form_state);

}

/**
 * Implements hook_submit()
 */
function cashbox_adjust_submit($form, &$form_state) {
	if(!isset($form_state['storage']['confirm'])) {
		$form_state['storage']['confirm'] = TRUE; // this will cause the form to be rebuilt, entering the confirm part of the form
		$form_state['rebuild'] = TRUE; // along with this
	} else {
		drupal_set_message(t('Transaction successful.'));
		global $user;
		$amount = number_format($form_state['values']['amount'], 2);
		$credited = ($amount > 0 ? _cashbox_fetch_data($form_state['values']['user']) : NULL);
		$debited = ($amount < 0 ? _cashbox_fetch_data($form_state['values']['user']) : NULL);
		$timestamp = time();
		$token = _cashbox_checksum(isset($credited->name) ? $credited->name : NULL, isset($debited->name) ? $debited->name : NULL, $user->name, $timestamp, substr($form_state['values']['message'],0,255), number_format(abs($amount), 2));
		db_insert('cashbox')
			->fields(array(
				'credited'		=> isset($credited->name) ? $credited->name : NULL,
				'debited'		=> isset($debited->name) ? $debited->name : NULL,
				'initiated'		=> $user->name,
				'timestamp' 	=> $timestamp,
				'message'	=> substr($form_state['values']['message'], 0, 255),
				'amount'	=> number_format(abs($amount), 2),
				'balance_creditee'	=> ($amount > 0 ? _cashbox_get_current_balance($credited->name)+$amount : 0),
				'balance_debitee'	=> ($amount < 0 ? _cashbox_get_current_balance($debited->name)+$amount : 0),
				'token'		=> $token,
			))->execute();
		if (isset($credited->master)) {
			error_log("Updating Master-Account Balance...");
			db_update('cashbox_masters')->fields(array('current_balance' => _cashbox_get_current_balance($credited->name)))->condition('cbm_id', $credited->uid, '=')->execute();
		}
		else if (isset($debited->master)) {
			db_update('cashbox_masters')->fields(array('current_balance' => _cashbox_get_current_balance($debited->name)))->condition('cbm_id', $debited->uid, '=')->execute();
			error_log("Updating Master-Account Balance...");
		}

		$form_state['redirect'] = 'cashbox/confirm/' . (!empty($credited) ? $credited->name : $debited->name) . '/' . $token;
	}
}


/**
 * Form function, called by drupal_get_form()
 * in cashbox_menu()
 */
function cashbox_transfer($form, &$form_state) {
	global $user;

#	print_r(cb_master_list());

	if(!isset($form_state['storage']['confirm'])) {

		$form['general'] = array(
			'#type'		=> 'fieldset',
			'#title'	=> t('Transaction data'),
		);

		/**
		 * Check, if the user may transfer from different accounts
		 */
		if (user_access('transfer cashbox other_balances') || user_access('cb_master transfer_from')) {
			$form['general']['fid'] = array(
					'#type' => 'textfield',
				'#title' => t('Sender'),
				'#size' => 20,
				'#maxlength' => 128,
				'#default_value' => $user->name,
				'#required'	=> true,
			);
			/**
			 * User may only transfer from regular accounts, but NOT away from a Master-Account:
			 */
			if (user_access('transfer cashbox other_balances') && !user_access('cb_master transfer_from'))
				$form['general']['fid']['#autocomplete_path'] = 'user/autocomplete';
			/**
			 * If the user-role permits transferring currency AWAY from a Master-Account but not from regular accounts, swap the autocomplete-path to the castrated autocomplete
			 */
			if (!user_access('transfer cashbox other_balances') && user_access('cb_master transfer_from')) 
				$form['general']['fid']['#autocomplete_path'] = 'cashbox/master/callback/autocomplete';
			/**
			 * User may transfer currency from regular accounts AS WELL AS AWAY from a Master-Account -> go for the full-fledged
			 */
			if (user_access('transfer cashbox other_balances') && user_access('cb_master transfer_from'))
				$form['general']['fid']['#autocomplete_path'] = 'cashbox/callback/transfer_autocomplete';
			
		}
		$form['general']['uid'] = array(
			'#type' => 'textfield',
			'#title' => t('Beneficiary'),
			'#size' => 20,
			'#maxlength' => 128,
			'#autocomplete_path' => 'cashbox/callback/transfer_autocomplete',
			'#required'	=> true,
		);
		$form['general']['amount'] = array(
			'#title' => t('Amount'),
			'#type'	=> 'textfield',
			'#field_prefix' => variable_get('cashbox_symbol', '$'),
			'#size'	=> 5,
			'#maxlength' => 7,
			'#default_value' => '0' . variable_get('cashbox_decimal', '.') . '00',
			'#required'	=> true,
		);
		$form['general']['message'] = array(
			'#title' => t('Comment'),
			'#type' => 'textfield',
			'#maxlength' => 255,
		);
		$form['general']['submit'] = array(
			'#value' => t('Save'),
			'#type' => 'submit',
		);

		return $form;
	}

	else {		/** confirm-page **/
		if (user_access('transfer cashbox other_balances')) {
			$form['general']['fid'] = array(
					'#type' => 'hidden',
				'#title' => t('Sender'),
				'#size' => 20,
				'#maxlength' => 128,
				'#default_value' => $user->name,
				'#autocomplete_path' => 'cashbox/callback/transfer_autocomplete',
				'#required'	=> true,
			);
		}
		$form['general']['uid'] = array(
			'#type' => 'hidden',
			'#title' => t('Beneficiary'),
			'#size' => 20,
			'#maxlength' => 128,
			'#autocomplete_path' => 'user/autocomplete',
			'#required'	=> true,
		);
		$form['general']['amount'] = array(
			'#title' => t('Amount'),
			'#type'	=> 'hidden',
			'#field_prefix' => variable_get('cashbox_symbol', '$'),
			'#size'	=> 5,
			'#maxlength' => 7,
			'#default_value' => '0' . variable_get('cashbox_decimal', '.') . '00',
			'#required'	=> true,
		);
		$form['general']['message'] = array(
			'#title' => t('Comment'),
			'#type' => 'hidden',
			'#maxlength' => 255,
		);

		drupal_add_css(drupal_get_path('module', 'cashbox') . '/cashbox.css');
		$row = array(array(
			(isset($form_state['values']['fid']) ? $form_state['values']['fid'] : $user->name),
			$form_state['values']['uid'],
			$form_state['values']['message'],
			array('data' => '<div class="cashbox_symbol">' . variable_get('cashbox_symbol', '$') . '</div><div class="cashbox_amount">' . number_format($form_state['values']['amount'], 2, variable_get('cashbox_decimal', '.'), variable_get('cashbox_thousand')) . '</div>', 'class' => 'cashbox_amount ' . ($form_state['values']['amount'] < 0 ? 'casbox_debit' : 'cashbox_credit')),
			array('data' => '<div class="cashbox_symbol">' . variable_get('cashbox_symbol', '$') . '</div><div class="cashbox_amount">' . number_format($form_state['values']['balance'], 2, variable_get('cashbox_decimal', '.'), variable_get('cashbox_thousand')) . '</div>', 'class' => 'cashbox_amount ' . ($form_state['values']['balance'] < 0 ? 'cashbox_debit' : 'cashbox_credit')),
		));
		$html = theme('table',  array(
			'header'        => array(t('Sender'), t('Beneficiary'), t('Comment'), t('Amount'), t('!user new balance', array('!user' => ($user->name == (isset($form_state['values']['fid']) ? $form_state['values']['fid'] : $user->name) ? 'Your' : $form_state['values']['fid'].'\'s')))),
			'empty'         => t('No transactions available'),
			'rows'          => $row,
			'sticky'        => true,
			'caption'       => t('Please double-check your data'),
		)

	);

		return confirm_form($form, 
			t('Are you sure you want to proceed?'),
			isset($_GET['destination']) ? $_GET['destination'] : "cashbox/transfer/normal", $html);
	}


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
function cashbox_transfer_validate($form, &$form_state) {
	global $user;
	$amount = str_replace(variable_get('cashbox_decimal', '.'), '.', str_replace(variable_get('cashbox_thousand', ','), '', $form_state['values']['amount']));
	$from_user = (empty($form_state['values']['fid']) ? $user : _cashbox_fetch_data($form_state['values']['fid']));
	$to_user = (empty($form_state['values']['uid']) ? NULL : _cashbox_fetch_data($form_state['values']['uid']));

	if (empty($from_user) || ($from_user != $user && !user_access('transfer cashbox other_balances')))
		form_set_error('fid', t('Sender :fid invalid.', array(':fid'=>$from_user)));

	if (empty($to_user))
		form_set_error('uid', t('Benificiary invalid.'));
	else if  ($from_user == $to_user)
		form_set_error('uid',  t('Nice try!'));

	if (!is_numeric($amount) || $amount <= 0)
		form_set_error('amount', t('Amount invalid.'));
	else {
		if ($amount > variable_get('cashbox_transaction_limit', 50) && !user_access('transfer cashbox may_transaction', $from_user))
			form_set_error('amount', t('!user may not exceeed the transaction limit of !symbol !number.', array(
				'!user' => ($user->uid == $from_user->uid ? 'You' : $from_user->name),
				'!symbol' => variable_get('cashbox_symbol', '$'),
				'!number' => number_format(variable_get('cashbox_transaction_limit', 50.00), 2, variable_get('cashbox_decimal', '.'), variable_get('cashbox_thousand')),
			)));
		else if (_cashbox_get_current_balance($from_user->name) - abs($amount) < -variable_get('cashbox_hardlimit', 50) && !user_access('transfer cashbox may_overdraw', $from_user))
			form_set_error('amount', t('Amount exceeds !user limit.', array('!user' => ($user->uid == $from_user->uid ? 'your' : $from_user->name.'\'s') )));
		else {
			form_set_value(array('#parents' => array('amount')), $amount, $form_state);
			form_set_value(array('#parents' => array('balance')), _cashbox_get_current_balance($from_user->name)-$amount, $form_state);
		}
	}
	form_set_value(array('#parents' => array('message')), filter_xss($form_state['values']['message'], array('em', 'strong', 'cite', 'blockquote', 'code', 'ul', 'ol', 'li', 'dl', 'dt', 'dd')), $form_state);

}

/**
 * Implements hook_submit()
 */
function cashbox_transfer_submit($form, &$form_state) {
	if(!isset($form_state['storage']['confirm'])) {
		$form_state['storage']['confirm'] = TRUE; // this will cause the form to be rebuilt, entering the confirm part of the form
		$form_state['rebuild'] = TRUE; // along with this
	}

	else {
		drupal_set_message(t('Transaction successful.'));
		global $user;
		$amount = number_format($form_state['values']['amount'], 2);
		$from_user = (empty($form_state['values']['fid']) ? $user : _cashbox_fetch_data($form_state['values']['fid']));
		$to_user = (empty($form_state['values']['uid']) ? $user : _cashbox_fetch_data($form_state['values']['uid']));
		$timestamp = time();
		$token = _cashbox_checksum($to_user->name, $from_user->name, ($from_user->name != $user->name $user->name : NULL), $timestamp, substr($form_state['values']['message'],0,255), $amount);
		db_insert('cashbox')
			->fields(array(
				'credited'		=> $to_user->name,
				'debited'		=> $from_user->name,
				'initiated'		=> ($from_user->name != $user->name  ? $user->name : NULL),
				'timestamp' 	=> $timestamp,
				'message'	=> substr($form_state['values']['message'], 0, 255),
				'amount'	=> $amount,
				'balance_creditee'	=> _cashbox_get_current_balance($to_user->name)+$amount,
				'balance_debitee'	=> _cashbox_get_current_balance($from_user->name)-$amount,
				'token'		=> $token,
			))->execute();
		$form_state['redirect'] = 'cashbox/confirm/' . ($from_user->name != $user->name ? $from_user->name : $user->name) . '/' . $token;
                if (isset($to_user->master)) 
			db_update('cashbox_masters')->fields(array('current_balance' => _cashbox_get_current_balance($to_user->name)))->condition('cbm_id', $to_user->uid, '=')->execute();
                 else if (isset($from_user->master))
			db_update('cashbox_masters')->fields(array('current_balance' => _cashbox_get_current_balance($from_user->name)))->condition('cbm_id', $from_user->uid, '=')->execute();
	}
	
			
}


/**
 * Display a confirmation-page for a transfer
 */
function _cashbox_transfer_confirm($d_user, $token) {
	global $user;

	$d_user = _cashbox_fetch_data($d_user);	

	drupal_add_css(drupal_get_path('module', 'cashbox') . '/cashbox.css');
	drupal_add_css(drupal_get_path('module', 'cashbox') . '/cashbox_print.css', array('media' => 'print'));

	$transaction= db_query('SELECT * FROM {cashbox} WHERE token = :token AND (credited = :user OR debited = :user OR initiated = :user)', array(':user' => $d_user->name, ':token' => $token))->fetchObject();

	if (!$transaction || ($d_user->uid != $user->uid && !user_access('transfer cashbox other_balances')))
		die(drupal_not_found());


	$html = theme('table',  array(
		'header'        => _cashbox_get_header(true, false, true),
		'empty'         => t('No transactions available'),
		'rows'          => array(_cashbox_format($transaction, $d_user, false, true, true)),
		'sticky'        => true,
		'caption'       => t('The following transaction has been completed successfully.'),
	));
 
	return $html;
}
