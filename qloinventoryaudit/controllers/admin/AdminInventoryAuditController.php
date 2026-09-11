<?php
/**
 * AdminInventoryAuditController
 *
 * Controller administrativo para auditoria de sobreposição de inventário.
 * Integra-se ao microserviço C++ local (porta 8107) para detecção de conflitos de ocupação.
 *
 * @author QloApps Engineering
 * @license AFL-3.0
 */

if (!defined('_PS_VERSION_')) {
    exit;
}

class AdminInventoryAuditController extends ModuleAdminController
{
    /** @var string URL do serviço C++ local de sobreposição */
    const OVERLAP_SERVICE_URL = 'http://127.0.0.1:8107/v1/inventory-audits/overlaps';

    /** @var int Timeout de requisição cURL em milissegundos */
    const CURL_TIMEOUT_MS = 800;

    /** @var int Timeout de conexão cURL em milissegundos */
    const CURL_CONNECT_TIMEOUT_MS = 600;

    /** @var int Limite máximo de reservas por lote (RN-005) */
    const MAX_BATCH_RESERVATIONS = 200;

    public function __construct()
    {
        $this->bootstrap = true;
        parent::__construct();
        $this->context = Context::getContext();
        $this->override_folder = '';
    }

    /**
     * Processa ações de submissão do formulário (exportação CSV, auditoria e carregamento do BD).
     */
    public function postProcess()
    {
        if (Tools::isSubmit('submitExportCsv')) {
            $this->processExportCsv();
        }

        parent::postProcess();
    }

    /**
     * Sanitiza campos para prevenir CSV Formula Injection (CWE-1236)
     * e garantir que apenas tipos escalares seguros sejam passados para fputcsv.
     *
     * @param mixed $value Valor a ser inserido na célula do CSV
     * @return string
     */
    protected function sanitizeCsvField($value)
    {
        if (is_array($value) || is_object($value)) {
            return '';
        }

        $str = trim((string) $value);
        // Se o valor iniciar com caracteres executáveis por planilhas, neutraliza com apóstrofo
        if (strlen($str) > 0 && in_array($str[0], ['=', '+', '-', '@', "\t", "\r"])) {
            return "'" . $str;
        }

        return $str;
    }

    /**
     * Exporta os resultados da auditoria em formato CSV com validação estrutural e sanitização contra CSV Injection.
     */
    protected function processExportCsv()
    {
        $rawConflicts = Tools::getValue('export_conflicts_json');
        $conflicts = json_decode($rawConflicts, true);

        if (!is_array($conflicts) || empty($conflicts)) {
            $this->errors[] = $this->l('Não há dados de conflitos disponíveis para exportação em CSV.');
            return;
        }

        // Teto de segurança defensivo contra exaustão de memória/processamento por payload adulterado
        if (count($conflicts) > 500) {
            $conflicts = array_slice($conflicts, 0, 500);
        }

        // Limpa buffers de saída anteriores para garantir que nenhum aviso/HTML seja emitido antes do CSV
        while (ob_get_level()) {
            ob_end_clean();
        }

        $filename = 'relatorio_conflitos_' . date('Ymd_His') . '.csv';

        header('Content-Type: text/csv; charset=UTF-8');
        header('Content-Disposition: attachment; filename="' . $filename . '"');
        header('Content-Transfer-Encoding: binary');
        header('Pragma: public');
        header('Expires: 0');
        header('Cache-Control: must-revalidate, post-check=0, pre-check=0');

        $output = fopen('php://output', 'w');
        // Adiciona BOM UTF-8 para correta interpretação de acentuação no Microsoft Excel / Calc
        fputs($output, "\xEF\xBB\xBF");

        // Passa parâmetros completos do fputcsv (delimiter, enclosure, escape) para compatibilidade com PHP 8.4+
        fputcsv($output, [
            $this->l('Quarto', null, false, false),
            $this->l('Reserva A', null, false, false),
            $this->l('Reserva B', null, false, false),
            $this->l('Início do Conflito', null, false, false),
            $this->l('Fim do Conflito', null, false, false),
            $this->l('Noites Sobrepostas', null, false, false),
            $this->l('Severidade', null, false, false),
            $this->l('Mensagem', null, false, false)
        ], ';', '"', "\\");

        foreach ($conflicts as $conflict) {
            // Ignora itens malformados que não sejam arrays associativos
            if (!is_array($conflict)) {
                continue;
            }

            $severity = isset($conflict['severity']) && in_array($conflict['severity'], ['HIGH', 'MEDIUM', 'LOW'], true)
                ? $conflict['severity']
                : 'MEDIUM';

            $nights = isset($conflict['overlap_nights']) && is_numeric($conflict['overlap_nights'])
                ? (int) $conflict['overlap_nights']
                : 0;

            fputcsv($output, [
                $this->sanitizeCsvField(isset($conflict['room_id']) ? $conflict['room_id'] : ''),
                $this->sanitizeCsvField(isset($conflict['reservation_a_id']) ? $conflict['reservation_a_id'] : ''),
                $this->sanitizeCsvField(isset($conflict['reservation_b_id']) ? $conflict['reservation_b_id'] : ''),
                $this->sanitizeCsvField(isset($conflict['overlap_start']) ? $conflict['overlap_start'] : ''),
                $this->sanitizeCsvField(isset($conflict['overlap_end']) ? $conflict['overlap_end'] : ''),
                $nights,
                $severity,
                $this->sanitizeCsvField(isset($conflict['message']) ? $conflict['message'] : '')
            ], ';', '"', "\\");
        }

        fclose($output);
        exit;
    }

    /**
     * Inicializa o conteúdo e prepara os dados para renderização na view Smarty.
     */
    public function initContent()
    {
        parent::initContent();

        $auditResult   = null;
        $errorMessage  = null;
        $rawJson       = trim(Tools::getValue('audit_batch_json'));
        $dateFrom      = Tools::getValue('db_date_from', date('Y-m-01'));
        $dateTo        = Tools::getValue('db_date_to', date('Y-m-t'));

        // Ação: Carregar reservas ativas do banco de dados por período
        if (Tools::isSubmit('submitLoadFromDb')) {
            $loadedJson = $this->loadReservationsFromDb($dateFrom, $dateTo, $errorMessage);
            if ($loadedJson !== false) {
                $rawJson = $loadedJson;
                $this->confirmations[] = $this->l('Lote de reservas carregado com sucesso do banco de dados.');
            }
        }

        // Ação: Executar auditoria de sobreposição via serviço C++
        if (Tools::isSubmit('submitRunAudit')) {
            if (empty($rawJson)) {
                $errorMessage = $this->l('Por favor, informe o JSON do lote de reservas.');
            } else {
                $parsedData = json_decode($rawJson, true);
                if (!$parsedData || !isset($parsedData['reservations']) || !is_array($parsedData['reservations'])) {
                    $errorMessage = $this->l('JSON de reservas inválido ou malformado.');
                } elseif (count($parsedData['reservations']) > self::MAX_BATCH_RESERVATIONS) {
                    // Validação de fronteira RN-005 antes de despachar para o C++
                    $errorMessage = sprintf(
                        $this->l('O lote de reservas excede o limite máximo permitido de %d registros (RN-005). Total enviado: %d.'),
                        self::MAX_BATCH_RESERVATIONS,
                        count($parsedData['reservations'])
                    );
                } else {
                    $auditResult = $this->callOverlapService($rawJson, $errorMessage);
                }
            }
        }

        if ($errorMessage) {
            $this->errors[] = $errorMessage;
        }

        $exportConflictsJson = '';
        if ($auditResult && isset($auditResult['conflicts'])) {
            $exportConflictsJson = json_encode($auditResult['conflicts']);
        }

        $this->context->smarty->assign([
            'auditResult'         => $auditResult,
            'auditError'          => $errorMessage,
            'rawJson'             => $rawJson,
            'demoFixtureJson'     => $this->getDemoFixtureJson(),
            'exportConflictsJson' => $exportConflictsJson,
            'dbDateFrom'          => $dateFrom,
            'dbDateTo'            => $dateTo,
            'currentAction'       => $this->context->link->getAdminLink('AdminInventoryAudit')
        ]);

        $tplPath = _PS_MODULE_DIR_ . $this->module->name . '/views/templates/admin/audit_dashboard.tpl';
        if (file_exists($tplPath)) {
            $this->content = $this->context->smarty->fetch($tplPath);
            $this->context->smarty->assign('content', $this->content);
        }
    }

    /**
     * Localiza e instancia templates Smarty do módulo para a interface administrativa.
     *
     * @param string $tpl_name Nome do arquivo de template
     * @return Smarty_Internal_Template
     */
    public function createTemplate($tpl_name)
    {
        $tplPath = _PS_MODULE_DIR_ . $this->module->name . '/views/templates/admin/' . $tpl_name;
        if (file_exists($tplPath) && $this->viewAccess()) {
            return $this->context->smarty->createTemplate($tplPath, $this->context->smarty);
        }

        return parent::createTemplate($tpl_name);
    }

    /**
     * Envia o lote de reservas ao serviço local em C++ via cURL.
     *
     * @param string $rawJson JSON payload
     * @param string &$errorMessage Mensagem de erro retornada por referência
     * @return array|null Resultado decodificado em caso de sucesso
     */
    protected function callOverlapService($rawJson, &$errorMessage)
    {
        // Geração de UUID v4 para rastreabilidade de requisições
        $corrId = sprintf(
            '%04x%04x-%04x-%04x-%04x-%04x%04x%04x',
            mt_rand(0, 0xffff), mt_rand(0, 0xffff),
            mt_rand(0, 0xffff),
            mt_rand(0, 0x0fff) | 0x4000,
            mt_rand(0, 0x3fff) | 0x8000,
            mt_rand(0, 0xffff), mt_rand(0, 0xffff), mt_rand(0, 0xffff)
        );

        $ch = curl_init(self::OVERLAP_SERVICE_URL);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_POST, true);
        curl_setopt($ch, CURLOPT_POSTFIELDS, $rawJson);
        curl_setopt($ch, CURLOPT_TIMEOUT_MS, self::CURL_TIMEOUT_MS);
        curl_setopt($ch, CURLOPT_CONNECTTIMEOUT_MS, self::CURL_CONNECT_TIMEOUT_MS);
        curl_setopt($ch, CURLOPT_HTTPHEADER, [
            'Content-Type: application/json',
            'X-Correlation-ID: ' . $corrId
        ]);

        $response = curl_exec($ch);
        $httpCode = (int) curl_getinfo($ch, CURLINFO_HTTP_CODE);
        $curlError = curl_error($ch);
        curl_close($ch);

        if ($response && $httpCode === 200) {
            $data = json_decode($response, true);
            if (is_array($data)) {
                $this->confirmations[] = $this->l('Auditoria de sobreposição executada com sucesso.');
                return $data;
            }
            $errorMessage = $this->l('Resposta do serviço C++ recebida em formato inválido.');
            return null;
        }

        if ($httpCode === 400 && $response) {
            $errData = json_decode($response, true);
            $detail = isset($errData['detail']) ? $errData['detail'] : (isset($errData['error']) ? $errData['error'] : $this->l('Erro de validação no lote.'));
            $errorMessage = sprintf($this->l('Falha de validação no lote (HTTP 400): %s'), $detail);
            return null;
        }

        // Contingência para serviço offline / timeout de 800ms
        $errorMessage = sprintf(
            $this->l('Serviço de auditoria C++ offline ou indisponível em %s (HTTP %d%s). Verifique se o binário overlap_service está em execução na porta 8107.'),
            self::OVERLAP_SERVICE_URL,
            $httpCode,
            $curlError ? ' - ' . $curlError : ''
        );

        return null;
    }

    /**
     * Consulta reservas ativas no banco de dados do QloApps e gera um lote JSON.
     * Enforça o limite da RN-005: se o período selecionado exceder MAX_BATCH_RESERVATIONS,
     * recusa o carregamento para evitar auditoria incompleta e falsos negativos de sobreposição.
     *
     * @param string $dateFrom Data inicial (YYYY-MM-DD)
     * @param string $dateTo Data final (YYYY-MM-DD)
     * @param string|null &$errorMessage Mensagem de erro retornada por referência
     * @return string|false JSON formatado ou falso em caso de erro
     */
    protected function loadReservationsFromDb($dateFrom, $dateTo, &$errorMessage = null)
    {
        if (!Validate::isDate($dateFrom) || !Validate::isDate($dateTo)) {
            $errorMessage = $this->l('Formato de período inválido para consulta ao banco de dados.');
            return false;
        }

        if (strtotime($dateFrom) > strtotime($dateTo)) {
            $errorMessage = $this->l('A data inicial não pode ser posterior à data final.');
            return false;
        }

        // Busca 1 registro a mais que o limite para detectar se o lote foi extrapolado sem truncamento cego
        $limitCheck = (int) self::MAX_BATCH_RESERVATIONS + 1;

        $sql = 'SELECT hbd.`id` as id_booking, hbd.`id_order`,
                       hbd.`id_room`,
                       DATE(hbd.`date_from`) as check_in, DATE(hbd.`date_to`) as check_out,
                       CONCAT(c.`firstname`, " ", c.`lastname`) as guest_name
                FROM `' . _DB_PREFIX_ . 'htl_booking_detail` hbd
                LEFT JOIN `' . _DB_PREFIX_ . 'customer` c ON (c.`id_customer` = hbd.`id_customer`)
                WHERE (hbd.`is_refunded` = 0 OR hbd.`is_refunded` IS NULL)
                  AND (hbd.`is_cancelled` = 0 OR hbd.`is_cancelled` IS NULL)
                  AND hbd.`date_from` >= "' . pSQL($dateFrom) . ' 00:00:00"
                  AND hbd.`date_to` <= "' . pSQL($dateTo) . ' 23:59:59"
                ORDER BY hbd.`date_from` ASC
                LIMIT ' . $limitCheck;

        try {
            $rows = Db::getInstance()->executeS($sql);
            if (!is_array($rows) || empty($rows)) {
                $errorMessage = sprintf($this->l('Nenhuma reserva ativa encontrada entre %s e %s.'), $dateFrom, $dateTo);
                return false;
            }

            // Se o período possuir mais reservas que o limite permitido por lote (RN-005), recusa para evitar falso negativo
            if (count($rows) > self::MAX_BATCH_RESERVATIONS) {
                $countSql = 'SELECT COUNT(*) FROM `' . _DB_PREFIX_ . 'htl_booking_detail` hbd
                             WHERE (hbd.`is_refunded` = 0 OR hbd.`is_refunded` IS NULL)
                               AND (hbd.`is_cancelled` = 0 OR hbd.`is_cancelled` IS NULL)
                               AND hbd.`date_from` >= "' . pSQL($dateFrom) . ' 00:00:00"
                               AND hbd.`date_to` <= "' . pSQL($dateTo) . ' 23:59:59"';
                $totalFound = (int) Db::getInstance()->getValue($countSql);
                if ($totalFound <= 0) {
                    $totalFound = count($rows);
                }

                $errorMessage = sprintf(
                    $this->l('O período selecionado contém %d reservas, excedendo o limite máximo permitido de %d registros por lote (RN-005). Por favor, reduza o intervalo de datas (ex: consulte semana a semana) para garantir uma auditoria íntegra e sem omissões.'),
                    $totalFound,
                    self::MAX_BATCH_RESERVATIONS
                );
                return false;
            }

            $reservations = [];
            foreach ($rows as $row) {
                $reservations[] = [
                    'reservation_id' => 'RES-' . ($row['id_order'] ? $row['id_order'] . '-' : '') . $row['id_booking'],
                    'room_id' => (string) $row['id_room'],
                    'check_in' => $row['check_in'],
                    'check_out' => $row['check_out'],
                    'guest_name' => !empty(trim($row['guest_name'])) ? trim($row['guest_name']) : 'Hóspede #' . $row['id_booking']
                ];
            }

            return json_encode([
                'audit_batch_id' => 'batch-db-' . date('Ymd-His'),
                'reservations' => $reservations
            ], JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE);
        } catch (Exception $e) {
            $errorMessage = sprintf($this->l('Erro ao consultar banco de dados: %s'), $e->getMessage());
            return false;
        }
    }

    /**
     * Retorna o JSON da fixture oficial de demonstração (Seção 9 do REQUIREMENTS.md).
     *
     * @return string
     */
    protected function getDemoFixtureJson()
    {
        return json_encode([
            'audit_batch_id' => 'fixture-batch-demo',
            'reservations' => [
                [
                    'reservation_id' => 'RES-001',
                    'room_id' => '101',
                    'check_in' => '2026-09-01',
                    'check_out' => '2026-09-05',
                    'guest_name' => 'Carlos Eduardo'
                ],
                [
                    'reservation_id' => 'RES-002',
                    'room_id' => '101',
                    'check_in' => '2026-09-03',
                    'check_out' => '2026-09-07',
                    'guest_name' => 'Mariana Lima'
                ],
                [
                    'reservation_id' => 'RES-003',
                    'room_id' => '101',
                    'check_in' => '2026-09-07',
                    'check_out' => '2026-09-10',
                    'guest_name' => 'Fernanda Rocha'
                ],
                [
                    'reservation_id' => 'RES-004',
                    'room_id' => '102',
                    'check_in' => '2026-09-01',
                    'check_out' => '2026-09-08',
                    'guest_name' => 'Pedro Alcantara'
                ],
                [
                    'reservation_id' => 'RES-005',
                    'room_id' => '102',
                    'check_in' => '2026-09-04',
                    'check_out' => '2026-09-07',
                    'guest_name' => 'Julia Martins'
                ]
            ]
        ], JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE);
    }
}
