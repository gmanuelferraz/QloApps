<?php
/**
 * QloInventoryAudit Module
 *
 * Módulo para auditoria de sobreposição de inventário e integridade física de quartos.
 *
 * @author QloApps Engineering
 * @license AFL-3.0
 */

if (!defined('_PS_VERSION_')) {
    exit;
}

class QloInventoryAudit extends Module
{
    public function __construct()
    {
        $this->name = 'qloinventoryaudit';
        $this->tab = 'hotel_reservation';
        $this->version = '1.0.0';
        $this->author = 'QloApps Engineering';
        $this->need_instance = 0;
        $this->bootstrap = true;

        parent::__construct();

        $this->displayName = $this->l('Auditor de Sobreposição de Inventário');
        $this->description = $this->l('Detecção de conflitos e duplas alocações de quartos físicos.');
        $this->ps_versions_compliancy = ['min' => '1.6', 'max' => _PS_VERSION_];
    }

    /**
     * Instala o módulo e registra a aba no menu administrativo.
     *
     * @return bool
     */
    public function install()
    {
        return parent::install() && $this->installTab();
    }

    /**
     * Desinstala o módulo e remove a aba do menu administrativo.
     *
     * @return bool
     */
    public function uninstall()
    {
        return $this->uninstallTab() && parent::uninstall();
    }

    /**
     * Cria e registra a aba administrativa 'AdminInventoryAudit' no menu de pedidos/reservas.
     *
     * @return bool
     */
    private function installTab()
    {
        $tabId = (int) Tab::getIdFromClassName('AdminInventoryAudit');
        if ($tabId) {
            return true;
        }

        $tab = new Tab();
        $tab->active = 1;
        $tab->class_name = 'AdminInventoryAudit';
        $tab->name = [];
        foreach (Language::getLanguages(true) as $lang) {
            $tab->name[$lang['id_lang']] = $this->l('Auditor de Conflitos');
        }

        $idParent = (int) Tab::getIdFromClassName('AdminParentOrders');
        if (!$idParent) {
            $idParent = (int) Tab::getIdFromClassName('AdminHotelReservation');
        }
        $tab->id_parent = $idParent;
        $tab->module = $this->name;

        return (bool) $tab->add();
    }

    /**
     * Remove a aba administrativa ao desinstalar o módulo.
     *
     * @return bool
     */
    private function uninstallTab()
    {
        $idTab = (int) Tab::getIdFromClassName('AdminInventoryAudit');
        if ($idTab) {
            $tab = new Tab($idTab);
            return (bool) $tab->delete();
        }
        return true;
    }
}
