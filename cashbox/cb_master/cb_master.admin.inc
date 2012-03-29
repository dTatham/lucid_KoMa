<?php

/**
 * Form function to set the master-accounts properties
 */
function cb_master_admin() {
	$result = db_query('SELECT cbm_id, account_name, account_group, description FROM {cashbox_masters}');
	$masters = array();
	$i = 0;
	foreach ($result as $cb_master) {
		$masters[$cb_master->cbm_id]['name'] = array('data' => $cb_master->account_name);
		$masters[$cb_master->cbm_id]['groups'] = '';

		$groups_raw = explode('|', $cb_master->account_group);
		for ($i = 0; $i < sizeof($groups_raw); $i++) {
			$masters[$cb_master->cbm_id]['groups'] .= user_role_load($groups_raw[$i])->name;
			if ($i != sizeof($groups_raw)-1)
				$masters[$cb_master->cbm_id]['groups'] .= ', ';
		}

		$masters[$cb_master->cbm_id]['desc'] = array('data' => $cb_master->description);
		$masters[$cb_master->cbm_id]['actions'] = array('data' => l(t('edit'), 'admin/config/cashbox.master/edit/' . $cb_master->cbm_id) . ' - ' . l(t('delete'), 'admin/config/cashbox.master/delete/' . $cb_master->cbm_id));
	}

	$m_head = array(t('Master Account'), t('Associated Group(s)'), t('Description'), t('Actions'));

	$page_array['cb_masters'] = array(
		'#markup'	=> l(t('New account'), 'admin/config/cashbox.master/new') . theme('table', array('header' => $m_head, 'rows' => $masters)),
	);
	return $page_array;

}

/**
 * Called by MENU_CALLBACK:
 * Delete entry (provide a form with cbm_id and a prompt)
 */
function cb_master_delete($form, &$form_state, $preseed) {
	$form['cb_master_delete'] = array(
		'#type'		=> 'fieldset',
		'#title'	=> t('Are you sure you want to delete this Master Account?'),
	);
	$form['cb_master_delete']['cbm_id'] = array(
		'#type'		=> 'hidden',
		'#value'	=> $preseed,
	);
	$form['cb_master_delete']['submit'] = array(
		'#type'		=> 'submit',
		'#value'	=> t('Delete'),
	);
	$form['cb_master_delete']['cancel'] = array(
		'#markup'	=> l(t('Cancel'), 'admin/config/cashbox/master'),
	);

	return $form;
}

function cb_master_delete_validate($form, &$form_state) {
	list($balance, $account) = db_query('SELECT current_balance, account_name FROM {cashbox_masters} WHERE cbm_id = :cbm_id', array(':cbm_id' => $form_state['values']['cbm_id']))->fetch(PDO::FETCH_NUM);
	if ($balance != 0.0)
		form_set_error('', t('Master Account "@account" has a non-zero balance. Please clear the account first.', array('@account' => $account))); 
}

function cb_master_delete_submit($form, &$form_state) {
	db_query('DELETE FROM cashbox_masters WHERE cbm_id = :cbm_id', array(':cbm_id' => $form_state['values']['cbm_id']));
	drupal_set_message(t('Master Account successfully delelted.'));
	$form_state['redirect'] = 'admin/config/cashbox/master';
}
/**
 * Called by MENU_Callback:
 * Provide the new / edit forms
 */
function cb_master_dispatch($form, &$form_state, $preseed = NULL) {
	/**
	 * if pressed is handed over (i.e. we have an edit-scenario here), load the default values for this master account
	 */
	if ($preseed != NULL)  {
		$preseed = db_query('SELECT cbm_id, account_name, account_group, description  from cashbox_masters WHERE cbm_id = :pid', array(':pid' => $preseed))->fetchAll();
		if (!empty($preseed))
			$preseed = array_pop($preseed);
	}
		
	$form['cb_master_new'] = array(
		'#type'		=> 'fieldset',
		'#title'	=> t('Create new Master Account'),
	);
	$form['cb_master_new']['cbm_name'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Name'),
		'#description'	=> t('Name of the new Master Account'),
		'#maxlength' 	=> 30,
		'#required'	=> TRUE,
		'#default_value'	=> isset($preseed->account_name) ? $preseed->account_name : '',
	);
	$form['cb_master_new']['cbm_group'] = array(
		'#type'		=> 'select',
		'#title'	=> t('Account Master'),
		'#description'	=> t('Select the user-role(s) associated with the account'),
		'#options'	=> user_roles(true),
		'#size'		=> 4,
		'#multiple'	=> TRUE,
		'#required'	=> TRUE,
	);
	/**
	 * match the role name in preseed to the corresponding role-id and select it per default
	 */
	if (isset($preseed->account_group)) {
		foreach (explode('|', $preseed->account_group) as $role)
			$form['cb_master_new']['cbm_group']['#default_value'][] = user_role_load($role)->name;
	}

	$form['cb_master_new']['cbm_desc'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Description'),
		'#description'	=> t('Description of the new Master Account'),
		'#default_value'        => isset($preseed->description) ? $preseed->description: '',
		'#maxlength'	=> 255,
	);
	$form['cb_master_new']['submit'] = array(
		'#type'		=> 'submit',
		'#value'	=> t('Save'),
	);
	$form['cb_master_new']['cancel'] = array(
		'#markup'	=> l(t('Cancel'), 'admin/config/cashbox/master'),
	);
	if ($preseed != NULL)
		$form['cb_master_new']['cbm_id'] = array(
			'#type'	=> 'hidden',
			'#value' => $preseed->cbm_id,
		);

	return $form;
}

function cb_master_dispatch_validate($form, &$form_state) {
	if (strpbrk($form_state['values']['cbm_name'], ' '))
		form_set_error('cbm_name', t('Master Account may not contain spaces'));
	if (!isset($form_state['values']['cbm_id'])) {
		$cbm_list = db_query('SELECT cbm_id FROM cashbox_masters WHERE account_name = :new_name', array(':new_name' => $form_state['values']['cbm_name']));
		if (sizeof($cbm_list->fetchAll()))
			form_set_error('cbm_name', t('Master-Account name is already taken'));
	}
}

function cb_master_dispatch_submit($form, &$form_state) {
	$cbm_groups = array();
	foreach ($form_state['values']['cbm_group'] as $gid)
		$cbm_groups[] = user_role_load($gid)->rid;

	/**
	 * if cbm_id is handed over by the form, we have an edit-scenario, therefore the table needs to be updated
	 */
	if (isset($form_state['values']['cbm_id'])) {
		$old_cbm = new cbMaster($form_state['values']['cbm_id']);
		db_update('cashbox_masters')->fields(array(
			'account_name'	=> $form_state['values']['cbm_name'],
			'account_group'	=> implode('|', $cbm_groups),
			'description'	=> $form_state['values']['cbm_desc'],
		))->condition('cbm_id', $form_state['values']['cbm_id'], '=')->execute();
		$affected = db_update('cashbox')->fields(array(
			'credited'	=> $form_state['values']['cbm_name'],
		))->condition('credited', $old_cbm->name, '=')->execute();
		$affected += db_update('cashbox')->fields(array(
			'debited'	=> $form_state['values']['cbm_name'],
		))->condition('debited', $old_cbm->name, '=')->execute(); 
		drupal_set_message(t('Master Account sucessfully updated!'));

		/**
		 * If the name of a Master Account has been changed, rebuild the checksums
		 */
		if (strcmp($old_cbm->name, $form_state['values']['cbm_name'])) {
			$results = db_select('cashbox', 'c')->fields('c', array('tid'))->condition(db_or()->condition('credited', $form_state['values']['cbm_name'])->condition('debited', $form_state['values']['cbm_name']))->execute();
			$batch_op = array();
			while ($foo = $results->fetchAssoc()) 
				$batch_op[] =  array('_cashbox_rebuild_checksum', array($foo['tid']));
			batch_set(array(
				'title' => t('Rebuilding Transaction-Checksums'),
				'progress_message' => t('Rebuilding Transaction-Checksums @current / @total'),
				'operations' => $batch_op,
			));
		}
	} else {
		db_insert('cashbox_masters')->fields(array(
			'account_name' 	=> $form_state['values']['cbm_name'],
			'account_group' => implode('|', $cbm_groups),
			'description'	=> $form_state['values']['cbm_desc'],
		))->execute();	
		drupal_set_message(t('New Master Account successfully created'));
	}
	$form_state['redirect'] = 'admin/config/cashbox/master';
 
}
